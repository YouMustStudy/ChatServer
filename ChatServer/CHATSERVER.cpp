#include "ChatServer.h"

#define LOGIN_ON

bool ChatServer::Initialize(short port)
{
	InitWSA(port);

	/// 로비 생성, 로비는 사실상 인원제한 없다.
	m_lobby = g_roomManager.CreateRoom("Lobby", INT_MAX);
	if (nullptr == m_lobby)
		return false;
	m_lobby->SetWeakPtr(m_lobby);

	return true;
}

void ChatServer::Run()
{
	std::string welcomeMsg{ "=====================\r\nWelcome To ChatServer\r\n=====================\r\n10자 이하 아이디로 로그인을 해주세요.\r\n/login [ID]" };
	std::cout << "[Start Accept]" << std::endl;

	/// Recv는 동기적으로 발생한다.
	/// Accept, Disconnect 또한 동기적으로 발생.
	/// => Recv용 소켓은 지역변수로 관리 가능.
	std::deque<SOCKET> recvSockets;
	recvSockets.emplace_back(m_listener);
	std::vector<fd_set> masterFdSets;
	std::vector<SOCKET> maxFds;
	bool dirtyFlag = true;

	masterFdSets.emplace_back();
	maxFds.emplace_back(0);

	char buffer[BUF_SIZE + 1];

	///select는 64인 제한이 있음.
	///64개 단위로 select를 반복적으로 호출, 인원제한을 해제한다.
	///매번 fd_set을 갱신하는 것은 오버헤드가 있으므로
	///갱신이 필요한 상황(Accept, Disconnect)에서만 dirtyFlag를 세팅, 다음 순회에서 갱신한다.
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
						if (INVALID_SOCKET != clientSocket)
						{
							/// 소켓 리스트에 등록
							recvSockets.emplace_back(clientSocket);
							dirtyFlag = true;

							UserPtr newUser = AddSession(clientSocket);
							assert(nullptr != newUser); /// 세션 생성 실패 - map인데 실패한다? 문제있음

							std::cout << "[Client Accept] - " << clientSocket << std::endl;
							newUser->SendChat(welcomeMsg);
#ifndef LOGIN_ON
							ProcessLogin(newUser, newUser->GetName());
#endif // LOGIN_ON
						}
					}
					/// Recv
					else
					{
						/// 데이터는 완전한 형태로 온다고 가정, timeout 혹은 논블럭킹으로 교체 필요.
						int recvLength = recv(readySoc, reinterpret_cast<char*>(buffer), BUF_SIZE, 0);
						UserPtr user = m_sessionTable[readySoc];
						assert(nullptr != user);
						/// 오류 예외처리 추가하자
						if (recvLength <= 0) ///종료처리
						{
							auto delPos = std::find(recvSockets.begin(), recvSockets.end(), readySoc);
							assert(recvSockets.end() != delPos);
							recvSockets.erase(delPos);
							dirtyFlag = true;
							assert(1 == EraseSession(readySoc)); ///싱글스레드에서 삭제실패? 문제있음

							/// 이후 태스크로 보낼것, 지연된 close, 
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

	if (true == user->GetIsLogin())
	{
		switch (cmd)
		{
		case CMD_HELP:
			ProcessHelp(user);
			break;

		case CMD_LOGIN:
			user->SendChat("이미 로그인되어있습니다.");
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
			ProcessCreateRoom(user, param[1].str(), std::stoi(param[2].str()));
			break;

		case CMD_ERROR:
			ProcessError(user);
			break;
		}
	}
	else
	{
		switch (cmd)
		{
		case CMD_LOGIN:
			ProcessLogin(user, param[1].str());
			break;

		default:
			user->SendChat("로그인을 먼저 해주세요.");
			break;
		}
	}
}

void ChatServer::DisconnectUser(UserPtr& user)
{
	RoomPtr curRoom = user->GetRoom();
	curRoom->Leave(user);

	/// 수정 필요(캡슐화)
	SOCKET userSocket = user->GetSocket();
	closesocket(userSocket);
	g_userManager.EraseUser(user);
	std::cout << "[USER LOGOUT] - " << user->GetName() << std::endl;
}

void ChatServer::ExchangeRoom(UserPtr &user, RoomPtr &enterRoom)
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
	else
	{
		user->SendChat("인원수 초과로 입장할 수 없습니다.");
	}
}

UserPtr ChatServer::AddSession(SOCKET socket)
{
	m_sessionTable.emplace(socket, new User(socket));
	if (nullptr != m_sessionTable[socket])
	{
		m_sessionTable[socket]->SetName(std::to_string(socket));
		return m_sessionTable[socket];
	}
	return nullptr;
}

size_t ChatServer::EraseSession(SOCKET socket)
{
	return m_sessionTable.erase(socket);
}

bool ChatServer::ProcessLogin(UserPtr &user, const std::string & userName)
{
	if (false == user->GetIsLogin())
	{
		if (true == g_userManager.AddUser(user, userName))
		{
			user->SetLogin(userName);
			ProcessHelp(user);
			m_lobby->Enter(user);
			return true;
		}
		user->SendChat("[입장실패] 중복인 ID입니다.");
	}
	return false;
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
	RoomPtr& oldRoom = user->GetRoom();
	if (nullptr != oldRoom)
	{
		if (true == oldRoom->IsSameIdx(roomIdx))
		{
			user->SendChat("현재 있는 방입니다.");
			return;
		}
	}

	RoomPtr newRoom = g_roomManager.GetRoom(roomIdx);
	if (nullptr == newRoom)
	{
		user->SendChat("없는 방번호입니다.");
		return;
	}
	ExchangeRoom(user, newRoom);
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

void ChatServer::ProcessMsg(const UserPtr& sender, const std::string& receiverName, const std::string& msg)
{
	if (sender->GetName() != receiverName)
	{
		sender->SendChat("본인에게는 전송할 수 없습니다.");
		return;
	}

	UserPtr receiver = g_userManager.GetUser(receiverName);
	if (nullptr == receiver)
	{
		sender->SendChat("유저를 찾을 수 없습니다.");
		return;
	}
	receiver->SendChat("[" + sender->GetName() + "'s Msg] " + msg);
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
	std::string& roomList = g_roomManager.GetRoomList();
	user->SendChat(roomList);
}

void ChatServer::ProcessCreateRoom(UserPtr & user, const std::string& roomName, int maxUser)
{
	if (maxUser < 2)
	{
		user->SendChat("방의 최대인원 수는 2 이상이어야 합니다.");
		return;
	}
	RoomPtr newRoom = g_roomManager.CreateRoom(roomName, maxUser);
	if (nullptr == newRoom)
	{
		user->SendChat("방 생성 실패!!");
		std::cout << "방 생성 실패" << std::endl;
		return;
	}
	ExchangeRoom(user, newRoom);
}

void ChatServer::ProcessHelp(const UserPtr & user)
{
	static std::string helpCmd{
"\r\n== 명령어 목록 == \r\n\
[입장] /join [방번호]\r\n\
[퇴장] /quit\r\n\
[쪽지] /msg [상대방] [메세지]\r\n\
[방 생성] /create [방이름] [최대인원]\r\n\
[방 목록] /roomlist\r\n\
[유저 목록] /userlist\r\n" };
	user->SendChat(helpCmd);
}

void ChatServer::ProcessError(const UserPtr & user)
{
	static std::string wrongCmd{ "잘못된 명령어 형식입니다." };
	user->SendChat(wrongCmd);
}