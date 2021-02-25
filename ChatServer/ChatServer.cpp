#include "ChatServer.h"


#define LOGIN_ON
bool ChatServer::Initialize(short port)
{
	Logger::SetLogPath(LOG_NORMAL, LOG_PATH);
	Logger::SetLogPath(LOG_WSA, WSALOG_PATH);

	if (false == InitWSA(port))
	{
		return false;
	}
	if (false == InitLobby())
	{
		return false;
	}
	return true;
}

void ChatServer::Run()
{
	std::string bufferoverMsg{ std::to_string(USERBUF_SIZE) + "�� �̻����� ���ڸ� �Է��� �� �����ϴ�." };
	std::string welcomeMsg{ "=====================\r\nWelcome To ChatServer\r\n=====================\r\n���� ���� " + std::to_string(MAX_IDLENGTH) + "����Ʈ ���� ���̵�� �α����� ���ּ���.\r\n/login [ID]" };
	Logger::Log("[Start Running]");

	// Recv�� ���������� ó���ȴ�.
	// Accept, Disconnect�� �߻��� ���� ������ ���������� ó�������ϴ�.
	// => Recv�� ������ ���������� ���� ����.
	std::deque<SOCKET> recvSockets; // ��ȸ�� �����鼭�� �߰������� �ݺ������� �Ͼ deque ����.
	recvSockets.emplace_back(m_listener);
	std::vector<fd_set> masterFdSets;
	std::vector<SOCKET> maxFds;
	bool dirtyFlag = true;

	masterFdSets.emplace_back();
	maxFds.emplace_back(0);

	//select�� 64�� ������ ����.
	//�̸� �ذ��ϱ� ���� ������ ���������� fd_set�� �߰�, select�� �ݺ������� ȣ���Ѵ�.
	//�Ź� fd_set�� �����ϴ� ���� ������尡 �����Ƿ�
	//������ �ʿ��� ��Ȳ(Accept, Disconnect)������ dirtyFlag�� ����, ���� ��ȸ���� �����Ѵ�.
	timeval timeout{ 0, 0 };
	fd_set copyFdSet;
	char buffer[BUF_SIZE + 1];
	while (true)
	{
		int loopCnt = (static_cast<int>(recvSockets.size()) - 1) / FD_SETSIZE + 1;
		// ���� ����Ʈ�� ���� �߻� �� FdSet ����.
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

		// ���� �ݺ������� select ȣ��.
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
					// Accept
					if (m_listener == readySoc)
					{
						int len = sizeof(SOCKADDR_IN);
						SOCKADDR_IN clientAddr;
						SOCKET clientSocket = accept(m_listener, reinterpret_cast<sockaddr*>(&clientAddr), &len);
						if (INVALID_SOCKET != clientSocket)
						{
							// recv�� ���������̳ʿ� ���
							recvSockets.emplace_back(clientSocket);
							dirtyFlag = true;

							UserPtr newUser = AddSession(clientSocket, clientAddr);
							assert(nullptr != newUser); // ���� ���� ���� - map�ε� �����Ѵ�? ��������

							Logger::Log("[SESSION IN] " + newUser->GetAddr());
							newUser->SendChat(welcomeMsg);

#ifndef LOGIN_ON
							ProcessLogin(newUser, newUser->GetName());
#endif // LOGIN_ON
						}
					}
					// Recv
					else
					{
						int recvLength = recv(readySoc, reinterpret_cast<char*>(buffer), BUF_SIZE, 0);
						UserPtr user = m_sessionTable[readySoc];
						assert(nullptr != user);

						if (recvLength <= 0) //����ó��
						{
							g_userManager.DisconnectUser(user); //�������̺� ����

							//select ���̺��� ����
							auto delPos = std::find(recvSockets.begin(), recvSockets.end(), readySoc);
							assert(recvSockets.end() != delPos);
							recvSockets.erase(delPos);
							dirtyFlag = true;

							//���� ���̺� ����
							Logger::Log("[SESSION Out] " + user->GetAddr());
							assert(1 == EraseSession(readySoc));

							if (recvLength < 0) //�����ڵ� �ڵ鸵
							{
								Logger::WsaLog(user->GetAddr().c_str(), WSAGetLastError());
							}
							continue;
						}

						int prevPos = max(0, static_cast<int>(user->m_data.size()) - 1);
						user->PushData(buffer, recvLength);
						while (true)
						{
							size_t cmdPos = user->m_data.find("\r\n", prevPos); // ���๮�� �߰� �� ��Ŷ ó��
							if (std::string::npos != cmdPos)
							{
								if (0 != cmdPos) // ���͸� ��Ÿ�� ġ�� ���� ó������ �ʴ´�.
								{
									ProcessPacket(user, user->m_data.substr(0, cmdPos));
								}
								user->m_data = user->m_data.substr(cmdPos + 2);
							}
							else break;
						}
						//�����ϰ� �޸𸮸� �����ϴ� ���� ���� ���� ������ �� �ִ� �ִ� ������ ũ�� ����.
						if (user->m_data.size() > USERBUF_SIZE)
						{
							user->m_data.clear();
							user->SendChat(bufferoverMsg);
						}
					}
				}
			}
		}
	}
}

void ChatServer::Terminate()
{
	Logger::Log("[Terminate Server]");
	//Listen ���� ���� �� WSA ����.
	closesocket(m_listener);
	WSACleanup();
}

bool ChatServer::InitWSA(short port)
{
	Logger::Log("[Initializing ChatServer - " + std::to_string(port) + "]");
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		Logger::Log("[Error] - WSAStartup()");
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
		Logger::Log("[Error] - Bind()");
		return false;
	}
	if (listen(m_listener, SOMAXCONN) == SOCKET_ERROR)
	{
		Logger::Log("[Error] - Listen()");
		return false;
	}
	return true;
}

bool ChatServer::InitLobby()
{
	// �κ� ����, �κ�� ��ǻ� �ο������� ����.
	m_lobby = g_roomManager.CreateRoom("Lobby", MAX_LOBBY_SIZE, false);
	if (nullptr == m_lobby)
		return false;
	m_lobby->SetWeakPtr(m_lobby);
	return true;
}

void ChatServer::ProcessPacket(UserPtr& user, std::string data)
{
	static std::string alreadyLoginMsg{ "�̹� �α��εǾ��ֽ��ϴ�." };
	static std::string plzLoginMsg{ "���� ���� " + std::to_string(MAX_IDLENGTH) + "����Ʈ ���� ���̵�� �α����� ���ּ���.\r\n/login [ID]" };

	if (nullptr == user)
	{
		return;
	}

	Logger::Log("[USER SEND] " + user->GetAddr() + " " + user->GetName() + " " + data);

	//��ɾ �Ľ� ��
	std::smatch param;
	int cmd = m_cmdParser.Parse(data, param);

	//�α���, ��ɾ� ���ο� ���� ���� ó�� �б�
	if (true == user->GetIsLogin())
	{
		switch (cmd)
		{
		case CMD_HELP:
			ProcessHelp(user);
			break;

		case CMD_LOGIN:
			user->SendChat(alreadyLoginMsg);
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

		case CMD_ALLUSERLIST:
			ProcessGetAllUserList(user);
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
		//�α����� �ȵ� ��� �α��θ� ó��.
		switch (cmd)
		{
		case CMD_LOGIN:
			ProcessLogin(user, param[1].str());
			break;

		default:
			user->SendChat(plzLoginMsg);
			break;
		}
	}
}

void ChatServer::ExchangeRoom(UserPtr &user, RoomPtr &enterRoom)
{
	static std::string manyPeopleMsg{ "�ο��� �ʰ��� ������ �� �����ϴ�." };
	if (nullptr == user ||
		nullptr == enterRoom)
	{
		return;
	}

	//���ο� �濡 ���� �� ���� ���� ������ ������� �۵��Ѵ�.
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
		//���ο� �濡 ���� ���� �� ���� �޼��� ����
		user->SendChat(manyPeopleMsg);
	}
}

UserPtr ChatServer::AddSession(SOCKET socket, SOCKADDR_IN addr)
{
	//Accept �߻� �� ���ο� ������ �����Ѵ�.
	m_sessionTable.emplace(socket, new User(socket, addr));
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

void ChatServer::ProcessLogin(UserPtr &user, const std::string & userName)
{
	static std::string errMsg{ "[�α��� ����] ID�� �ߺ��˴ϴ�." };
	static std::string longIdMsg{ "[�α��� ����] ID�� " + std::to_string(MAX_IDLENGTH) + "����Ʈ ���Ͽ��� �մϴ�." };

	if (nullptr == user)
	{
		return;
	}

	if (userName.size() > MAX_IDLENGTH)
	{
		user->SendChat(longIdMsg);
		return;
	}
	if (false == user->GetIsLogin())
	{
		//���� ���̺� �߰��� �� ���� �޼��� ���, �κ� ����.
		if (true == g_userManager.AddUser(user, userName))
		{
			Logger::Log("[USER LOGIN] " + user->GetAddr() + " " + user->GetName());
			ProcessHelp(user);
			m_lobby->Enter(user);
			return;
		}
		user->SendChat(errMsg);
	}
	return;
}

void ChatServer::ProcessChat(const UserPtr &sender, const std::string &msg)
{
	if (nullptr == sender)
	{
		return;
	}

	const RoomPtr userRoom = sender->GetRoom();
	if (nullptr != userRoom)
	{
		userRoom->SendChat(sender, msg);
	}
}

void ChatServer::ProcessJoin(UserPtr &user, int roomIdx)
{
	static std::string alreadyExistMsg{ "���� �ִ� ���Դϴ�." };
	static std::string noExistMsg{ "���� ���ȣ�Դϴ�." };

	if (nullptr == user)
	{
		return;
	}

	RoomPtr oldRoom = user->GetRoom();
	if (nullptr != oldRoom)
	{
		//���� �������� �������� ����.
		if (true == oldRoom->IsSameIdx(roomIdx))
		{
			user->SendChat(alreadyExistMsg);
			return;
		}
	}


	RoomPtr newRoom = g_roomManager.GetRoom(roomIdx);
	if (nullptr == newRoom)
	{
		//�ε����� �ش��ϴ� ���� ���ٸ� ���� �޼��� ����.
		user->SendChat(noExistMsg);
		return;
	}
	ExchangeRoom(user, newRoom);
}

void ChatServer::ProcessQuit(UserPtr &user)
{
	static std::string errMsg{ "�κ񿡼��� ������ �� �����ϴ�." };
	if (nullptr == user)
	{
		return;
	}

	RoomPtr userRoom = user->GetRoom();
	if (nullptr == userRoom)
	{
		return;
	}

	//�κ� ������ �������� ���� ����.
	//������ -> �κ���� �̵��� ó��.
	if (m_lobby == userRoom)
	{
		user->SendChat(errMsg);
		return;
	}
	ExchangeRoom(user, m_lobby);
}

void ChatServer::ProcessMsg(const UserPtr& sender, const std::string& receiverName, const std::string& msg)
{
	static std::string cantSendSamePeopleMsg{ "���ο��Դ� ������ �� �����ϴ�." };
	static std::string cantFindPeopleMsg{ "������ ã�� �� �����ϴ�." };
	if (nullptr == sender)
	{
		return;
	}

	// ���ο��Դ� ������ �� ����.
	if (sender->GetName() == ("[" + receiverName + "]"))
	{
		sender->SendChat(cantSendSamePeopleMsg);
		return;
	}

	// �ش� �̸��� ���� ������ Ž��
	const UserPtr receiver = g_userManager.GetUser(receiverName);
	if (nullptr == receiver)
	{
		sender->SendChat(cantFindPeopleMsg);
		return;
	}
	receiver->SendChat("[MESSAGE FROM] " + sender->GetName() + " " + msg);
}

void ChatServer::ProcessGetUserList(const UserPtr &user)
{
	if (nullptr == user)
	{
		return;
	}

	//������ �ִ� ���� ���� ��� ȹ�� �� ����
	const RoomPtr userRoom = user->GetRoom();
	if (nullptr != userRoom)
	{
		std::string userList = userRoom->GetUserList();
		user->SendChat(userList);
	}
}

void ChatServer::ProcessGetRoomList(const UserPtr & user)
{
	if (nullptr == user)
	{
		return;
	}

	//roomManager�� ������ �� ��� ȹ�� �� ����.
	std::string roomList = g_roomManager.GetRoomList();
	user->SendChat(roomList);
}

void ChatServer::ProcessGetAllUserList(const UserPtr & user)
{
	if (nullptr == user)
	{
		return;
	}
	//������ ��� ������ ��� ȹ�� �� ����.
	user->SendChat(g_userManager.GetUserList());
}

void ChatServer::ProcessCreateRoom(UserPtr & user, const std::string& roomName, int maxUser)
{
	static std::string errMsg{ "���� �ִ��ο� ���� " + std::to_string(MINUSER_NUM) + " �̻�, " + std::to_string(MAXUSER_NUM) + " �����̾�� �մϴ�." };
	if (nullptr == user)
	{
		return;
	}
	if (MINUSER_NUM > maxUser ||
		MAXUSER_NUM < maxUser
		)
	{
		user->SendChat(errMsg);
		return;
	}
	//�� ������ ���� ����
	RoomPtr newRoom = g_roomManager.CreateRoom(roomName, maxUser);
	if (nullptr == newRoom)
	{
		Logger::Log("[Error] - �� ���� ����");
		user->SendChat("�� ������ �����Ͽ����ϴ�.");
		return;
	}
	ExchangeRoom(user, newRoom);

}

void ChatServer::ProcessHelp(const UserPtr & user)
{
	//���� �޼��� ����
	static std::string helpCmd{
"\r\n== ��ɾ� ��� == \r\n\
[����] /help\r\n\
[����] /join [���ȣ]\r\n\
[����] /quit\r\n\
[����] /msg [����] [�޼���]\r\n\
[�� ����] /create [���̸�] [�ִ��ο�]\r\n\
[�� ���] /roomlist\r\n\
[�� �� ���� ���] /userlist\r\n\
[���� �� ���� ���] /alluserlist\r\n" };
	user->SendChat(helpCmd);
}

void ChatServer::ProcessError(const UserPtr & user)
{
	if (nullptr == user)
	{
		return;
	}
	//��ɾ� ��Ͽ� ���� ��ɾ ���� �� ó��.
	static std::string wrongCmd{ "�߸��� ��ɾ� �����Դϴ�." };
	user->SendChat(wrongCmd);
}