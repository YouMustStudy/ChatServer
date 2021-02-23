#include "ChatServer.h"


bool ChatServer::Initialize(short port)
{
	InitWSA(port);

	/// �κ� ����
	m_lobby = m_roomMgr.CreateRoom("Lobby");
	if (nullptr == m_lobby)
		return false;

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

						/// ���� ����Ʈ�� ���
						recvSockets.emplace_back(clientSocket);
						dirtyFlag = true;

						std::cout << "Client Accept - " << clientSocket << std::endl;
						send(clientSocket, welcomeMsg, int(strlen(welcomeMsg)) + 1, 0);

						//Non-Blocking Socket
						//u_long nonBlockingMode = 1;
						//ioctlsocket(clientSocket, FIONBIO, &nonBlockingMode);

						m_userTable.emplace(std::make_pair(clientSocket, new User(clientSocket)));
						m_userTable[clientSocket]->m_name = std::to_string(clientSocket);
						if (true == m_lobby->Enter(m_userTable[clientSocket]))
						{
							m_userTable[clientSocket]->m_room = m_lobby;
						}
					}
					/// Recv
					else
					{
						/// �����ʹ� ������ ���·� �´ٰ� ����, timeout Ȥ�� ���ŷ���� ��ü �ʿ�.
						int recvLength = recv(readySoc, reinterpret_cast<char*>(buffer), BUF_SIZE, 0);
						UserPtr user = m_userTable[readySoc];
						assert(user);

						if (recvLength == 0) ///����ó��
						{
							auto delPos = std::find(recvSockets.begin(), recvSockets.end(), readySoc);
							assert(recvSockets.end() != delPos);
							recvSockets.erase(delPos);
							dirtyFlag = true;
							DisconnectUser(user);
							continue;
						}

						/// ���� �� �����͸� ���ڿ�ȭ. -> ���� ���� �ʿ�(��Ŷ)
						std::string& data = user->m_data;
						for (int i = 0; i < recvLength; ++i)
						{
							if (buffer[i] == VK_BACK)
								data.pop_back();
							else
								data.push_back(buffer[i]);
						}

						while (true)
						{
							size_t cmdPos = data.find("\r\n"); /// ���๮�� �߰� �� ��Ŷ ó��
							if (std::string::npos != cmdPos)
							{
								ProcessPacket(user, data.substr(0, cmdPos));
								data = data.substr(cmdPos + 2);
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

	case CMD_ERROR:
		ProcessError(user);
		break;
	}
}

void ChatServer::DisconnectUser(UserPtr& user)
{
	RoomPtr curRoom = user->GetRoom();
	curRoom->Leave(user);

	/// ���� �ʿ�(ĸ��ȭ)
	closesocket(user->m_socket);
	std::cout << "Client Out - " << user->m_name << std::endl;
	m_userTable.erase(user->m_socket);
}

void ChatServer::ExchangeRoom(UserPtr & user, RoomPtr &enterRoom)
{
	if (nullptr == enterRoom)
	{
		return;
	}
	RoomPtr oldRoomPtr = user->m_room;
	if (true == enterRoom->Enter(user))
	{
		oldRoomPtr->Leave(user);
	}
}

void ChatServer::ProcessChat(const UserPtr &sender, const std::string &msg)
{
	const RoomPtr roomPtr = sender->GetRoom();
	if (nullptr != roomPtr)
	{
		roomPtr->SendChat(sender, msg);
	}
}

void ChatServer::ProcessJoin(UserPtr &user, int roomIdx)
{
	RoomPtr newRoomPtr = m_roomMgr.GetRoom(roomIdx);
	if (nullptr == newRoomPtr)
	{
		user->SendChat("���� ���Դϴ�.");
		return;
	}
	ExchangeRoom(user, newRoomPtr);
}

void ChatServer::ProcessQuit(UserPtr &user)
{
	if (m_lobby == user->m_room)
	{
		user->SendChat("�κ񿡼��� ������ �� �����ϴ�.");
		return;
	}
	ExchangeRoom(user, m_lobby);
}

void ChatServer::ProcessMsg(const UserPtr& sender, const UserPtr& receiver, const std::string& msg)
{
	sender->SendChat("�κ񿡼��� ������ �� �����ϴ�.");
}

void ChatServer::ProcessMsg(const UserPtr& sender, const std::string& receiverName, const std::string& msg)
{
	//UserPtr receiver = 
	sender->SendChat("�κ񿡼��� ������ �� �����ϴ�.");
}

void ChatServer::ProcessGetUserList(const UserPtr &user)
{
	const RoomPtr roomPtr = user->GetRoom();
	if (nullptr != roomPtr)
	{
		std::string& userList = roomPtr->GetUserList();
		user->SendChat(userList);
	}
}

void ChatServer::ProcessGetRoomList(const UserPtr & user)
{
	std::string& roomList = m_roomMgr.GetRoomList();
	user->SendChat(roomList);
}

void ChatServer::ProcessError(UserPtr & user)
{
	user->SendChat("�߸��� ��ɾ� �����Դϴ�.");
}
