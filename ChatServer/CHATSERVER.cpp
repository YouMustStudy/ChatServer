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

	fd_set masterFds, copyFds;
	FD_ZERO(&masterFds);
	FD_SET(m_listener, &masterFds);
	SOCKET maxFd = m_listener;

	char buffer[BUF_SIZE + 1];
	while (true)
	{
		copyFds = masterFds;
		int numFd = select(static_cast<int>(maxFd) + 1, &copyFds, 0, 0, 0);
		assert(numFd > 0);

		for (int readySoc = 0; readySoc < maxFd + 1; ++readySoc)
		{
			if (FD_ISSET(readySoc, &copyFds))
			{
				/// Accept
				if (m_listener == readySoc)
				{
					int len = sizeof(SOCKADDR_IN);
					SOCKADDR_IN clientAddr;
					SOCKET clientSocket = accept(m_listener, reinterpret_cast<sockaddr*>(&clientAddr), &len);
					FD_SET(clientSocket, &masterFds);
					maxFd = max(maxFd, clientSocket);

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
						FD_CLR(readySoc, &masterFds);
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

}

void ChatServer::ProcessError(UserPtr & user)
{
	user->SendChat("�߸��� ��ɾ� �����Դϴ�.");
}
