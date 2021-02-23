#include "ChatServer.h"


bool ChatServer::Initialize(short port)
{
	InitWSA(port);

	/// 로비 생성
	m_lobby = m_roomMgr.CreateRoom("Lobby", INT_MAX);
	if (nullptr == m_lobby)
		return false;
	m_lobby->SetWeakPtr(m_lobby);

	return true;
}

void ChatServer::Run()
{
	char welcomeMsg[]("=====================\r\nWelcome To ChatServer\r\n=====================\r\n");
	std::cout << "[Start Accept]" << std::endl;

	std::deque<SOCKET> recvSockets;
	recvSockets.emplace_back(m_listener);
	std::vector<fd_set> masterFdSets;
	std::vector<SOCKET> maxFds;
	bool dirtyFlag = true;

	masterFdSets.emplace_back();
	maxFds.emplace_back(0);

	char buffer[BUF_SIZE + 1];
	
	timeval timeout{ 0, 0 };
	fd_set copyFdSet;
	while (true)
	{
		int loopCnt = (static_cast<int>(recvSockets.size()) - 1) / FD_SETSIZE + 1;
		/// 소켓 리스트에 변경 발생 시 FdSet 갱신.
		if (true == dirtyFlag)
		{
			while (masterFdSets.size() < loopCnt)
			{
				masterFdSets.emplace_back();
				maxFds.emplace_back(0);
			}

			for (int fdSetIdx = 0; fdSetIdx < masterFdSets.size(); ++fdSetIdx)
			{
				FD_ZERO(&masterFdSets[fdSetIdx]);
				maxFds[fdSetIdx] = 0;
				for (int socIdx = FD_SETSIZE*fdSetIdx; socIdx < recvSockets.size(); ++socIdx)
				{
					FD_SET(recvSockets[socIdx], &masterFdSets[fdSetIdx]);
					maxFds[fdSetIdx] = max(maxFds[fdSetIdx], recvSockets[socIdx]);
				}
			}
			dirtyFlag = false;
		}

		for (int fdSetNum = 0; fdSetNum < loopCnt; ++fdSetNum)
		{
			copyFdSet = masterFdSets[fdSetNum];
			int numFd = select(static_cast<int>(maxFds[fdSetNum]) + 1, &copyFdSet, 0, 0, &timeout);
			assert(numFd >= 0);
			if (numFd == 0)
			{
				continue;
			}

			for (int readySoc = 0; readySoc < maxFds[fdSetNum] + 1; ++readySoc)
			{
				if (FD_ISSET(readySoc, &copyFdSet))
				{
					/// Accept
					if (m_listener == readySoc)
					{
						int len = sizeof(SOCKADDR_IN);
						SOCKADDR_IN clientAddr;
						SOCKET clientSocket = accept(m_listener, reinterpret_cast<sockaddr*>(&clientAddr), &len);

						/// 소켓 리스트에 등록
						recvSockets.emplace_back(clientSocket);
						dirtyFlag = true;

						std::cout << "Client Accept - " << clientSocket << std::endl;
						send(clientSocket, welcomeMsg, int(strlen(welcomeMsg)) + 1, 0);

						m_userTable.emplace(std::make_pair(clientSocket, new User(clientSocket)));
						m_userTable[clientSocket]->SetName(std::to_string(clientSocket));
						ProcessHelp(m_userTable[clientSocket]);
						m_lobby->Enter(m_userTable[clientSocket]);
					}
					/// Recv
					else
					{
						/// 데이터는 완전한 형태로 온다고 가정, timeout 혹은 논블럭킹으로 교체 필요.
						int recvLength = recv(readySoc, reinterpret_cast<char*>(buffer), BUF_SIZE, 0);
						UserPtr user = m_userTable[readySoc];
						assert(nullptr != user);

						if (recvLength == 0) ///종료처리
						{
							auto delPos = std::find(recvSockets.begin(), recvSockets.end(), readySoc);
							assert(recvSockets.end() != delPos);
							recvSockets.erase(delPos);
							dirtyFlag = true;
							DisconnectUser(user);
							continue;
						}

						user->PushData(buffer, recvLength);

						while (true)
						{
							size_t cmdPos = user->m_data.find("\r\n"); /// 개행문자 발견 시 패킷 처리
							if (std::string::npos != cmdPos)
							{
								if (0 != cmdPos) /// 엔터만 연타로 치는 경우 견제
								{
									ProcessPacket(user, user->m_data.substr(0, cmdPos));
								}
								user->m_data = user->m_data.substr(cmdPos + 2);
							}
							else break;
						}
					}
				}
			}
		}
	}
}

void ChatServer::Terminate()
{
	closesocket(m_listener);
	WSACleanup();
}

bool ChatServer::InitWSA(short port)
{
	std::cout << "[Initializing ChatServer" << " - " << port << "]" << std::endl;
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "Error - WSAStartup()" << std::endl;
		return false;
	}

	SOCKADDR_IN addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	m_listener = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(m_listener, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		std::cout << "Error - Bind()" << std::endl;
		return false;
	}

	if (listen(m_listener, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "Error - Listen()" << std::endl;
		return false;
	}

	return true;
}

void ChatServer::ProcessPacket(UserPtr& user, std::string data)
{
	std::smatch param;
	int cmd = m_cmdParser.Parse(data, param);
	switch (cmd)
	{
	case CMD_HELP:
		ProcessHelp(user);
		break;

	case CMD_CHAT:
		ProcessChat(user, data);
		break;

	case CMD_JOIN:
		ProcessJoin(user, std::stoi(param[1].str()));
		break;

	case CMD_QUIT:
		ProcessQuit(user);
		break;

	case CMD_MSG:
		ProcessMsg(user, param[1].str(), param[2].str());
		break;

	case CMD_USERLIST:
		ProcessGetUserList(user);
		break;

	case CMD_ROOMLIST:
		ProcessGetRoomList(user);
		break;

	case CMD_CREATEROOM:
	{
		int maxUser = std::stoi(param[2].str());
		if (maxUser > 0)
		{
			ProcessCreateRoom(user, param[1].str(), maxUser);
		}
	}
		break;

	case CMD_ERROR:
		ProcessError(user);
		break;
	}
}

void ChatServer::DisconnectUser(UserPtr& user)
{
	RoomPtr curRoom = user->GetRoom();
	curRoom->Leave(user);

	/// 수정 필요(캡슐화)
	SOCKET userSocket = user->GetSocket();
	closesocket(userSocket);
	std::cout << "Client Out - " << user->GetName() << std::endl;
	m_userTable.erase(userSocket);
}

void ChatServer::ExchangeRoom(UserPtr & user, RoomPtr &enterRoom)
{
	if (nullptr == enterRoom)
	{
		return;
	}
	RoomPtr oldRoomPtr = user->GetRoom();
	if (true == enterRoom->Enter(user))
	{
		if (nullptr != oldRoomPtr) 
		{
			oldRoomPtr->Leave(user);
		}
	}
}

void ChatServer::ProcessChat(const UserPtr &sender, const std::string &msg)
{
	const RoomPtr userRoom = sender->GetRoom();
	if (nullptr != userRoom)
	{
		userRoom->SendChat(sender, msg);
	}
}

void ChatServer::ProcessJoin(UserPtr &user, int roomIdx)
{
	RoomPtr newRoomPtr = m_roomMgr.GetRoom(roomIdx);
	if (nullptr == newRoomPtr)
	{
		user->SendChat("없는 방입니다.");
		return;
	}
	ExchangeRoom(user, newRoomPtr);
}

void ChatServer::ProcessQuit(UserPtr &user)
{
	RoomPtr userRoom = user->GetRoom();
	if (nullptr == userRoom)
	{
		return;
	}

	if (m_lobby == userRoom)
	{
		user->SendChat("로비에서는 나가실 수 없습니다.");
		return;
	}
	ExchangeRoom(user, m_lobby);
}

void ChatServer::ProcessMsg(const UserPtr& sender, const UserPtr& receiver, const std::string& msg)
{
	sender->SendChat("로비에서는 나가실 수 없습니다.");
}

void ChatServer::ProcessMsg(const UserPtr& sender, const std::string& receiverName, const std::string& msg)
{
	//UserPtr receiver = 
	sender->SendChat("로비에서는 나가실 수 없습니다.");
}

void ChatServer::ProcessGetUserList(const UserPtr &user)
{
	const RoomPtr userRoom = user->GetRoom();
	if (nullptr != userRoom)
	{
		std::string& userList = userRoom->GetUserList();
		user->SendChat(userList);
	}
}

void ChatServer::ProcessGetRoomList(const UserPtr & user)
{
	std::string& roomList = m_roomMgr.GetRoomList();
	user->SendChat(roomList);
}

void ChatServer::ProcessCreateRoom(UserPtr & user, const std::string& roomName, int maxUser)
{
	RoomPtr newRoom = m_roomMgr.CreateRoom(roomName, maxUser);
	if (nullptr == newRoom)
	{
		user->SendChat("방 생성 실패!!");
		return;
	}
	ExchangeRoom(user, newRoom);
}

void ChatServer::ProcessHelp(const UserPtr & user)
{
	static std::string helpCmd{
"== 명령어 목록 == \r\n\
[입장] /join [방번호]\r\n\
[퇴장] /quit\r\n\
[쪽지] /msg [상대방] [메세지] - 미구현\r\n\
[방 생성] /create [방이름] [최대인원]\r\n\
[방 목록] /roomlist\r\n\
[유저 목록] /userlist\r\n"	};
	user->SendChat(helpCmd);
}

void ChatServer::ProcessError(const UserPtr & user)
{
	static std::string wrongCmd{ "잘못된 명령어 형식입니다." };
	user->SendChat(wrongCmd);
}
