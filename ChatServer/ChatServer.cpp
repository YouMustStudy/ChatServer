#include "ChatServer.h"
thread_local int thread_id;
std::atomic_int abc;

#define LOGIN_ON
ChatServer& ChatServer::Instance()
{
	//�̱��� ��ü ���� �� ȣ��.
	static ChatServer* chatServer = new ChatServer();
	return *chatServer;
}
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

	InitMultiThread();

	return true;
}

void ChatServer::Run()
{
	std::string bufferoverMsg{ std::to_string(USERBUF_SIZE) + "�� �̻����� ���ڸ� �Է��� �� �����ϴ�." };
	Logger::Log("[Start Running]");

	fd_set copyFdSet;
	fd_set masterFdSet;

	SessionTable sessionTable;
	FD_ZERO(&masterFdSet);
	FD_SET(m_listener, &masterFdSet);
	SOCKET maxFd = m_listener;

	timeval timeout{ 0, 500 };
	char buffer[BUF_SIZE + 1];
	char addrArray[INET_ADDRSTRLEN];
	while (true)
	{
		// ��ĥ recvThread�� ������ ���� ������� ��ħ.
		while (false == m_sessionQueue.empty())
		{
			SessionTable* stocks = nullptr;
			while (false == m_sessionQueue.try_pop(stocks)) {};
			for (const auto& session : *stocks)
			{
				sessionTable[session.first] = session.second;
			}
			delete stocks;

			//�ٽ� �缳��.
			FD_ZERO(&masterFdSet);
			FD_SET(m_listener, &masterFdSet);
			maxFd = m_listener;
			for (const auto& session : sessionTable)
			{
				FD_SET(session.first, &masterFdSet);
				maxFd = max(session.first, maxFd);
			}
		}

		// -1�� ListenSocket�� ����� ��.
		// 63�� ������ recvThread folk.
		if (FD_SETSIZE - 1 <= sessionTable.size())
		{
			while (FD_SETSIZE - 1 <= sessionTable.size())
			{
				int cnt = 0;
				SessionTable* folk = new SessionTable;
				for (auto iter = sessionTable.begin(); iter != sessionTable.end();)
				{
					(*folk)[iter->first] = iter->second;
					sessionTable.erase(iter++);
					if (FD_SETSIZE - 1 == ++cnt)
					{
						break;
					}
				}
				m_recvThreads.emplace_back(&ChatServer::RecvThread, this, folk);
			}
			//�ٽ� �缳��.
			FD_ZERO(&masterFdSet);
			FD_SET(m_listener, &masterFdSet);
			maxFd = m_listener;
			for (const auto& session : sessionTable)
			{
				FD_SET(session.first, &masterFdSet);
				maxFd = max(session.first, maxFd);
			}
		}

		//���� ó����.
		timeout.tv_usec = SELECT_TIMEOUT;
		copyFdSet = masterFdSet;
		int numFd = select(static_cast<int>(maxFd) + 1, &copyFdSet, 0, 0, &timeout);
		assert(numFd >= 0);
		if (numFd != 0)
		{
			for (int readySoc = 0; readySoc < maxFd + 1; ++readySoc)
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
							maxFd = max(clientSocket, maxFd);
							FD_SET(clientSocket, &masterFdSet);
							// recv�� ���������̳ʿ� ���
							sessionTable.emplace(clientSocket, Session());
							// ���ǿ� ���� �߰�
							sessionTable[clientSocket].socket = clientSocket;
							sessionTable[clientSocket].addr.clear();
							sessionTable[clientSocket].buffer.clear();

							// ���� ���� �̺�Ʈ ������
							inet_ntop(AF_INET, &clientAddr.sin_addr, addrArray, INET_ADDRSTRLEN);
							sessionTable[clientSocket].addr = "[" + std::string(addrArray) + ":" + std::to_string(ntohs(clientAddr.sin_port)) + "]";
							std::string* addrText = new std::string(sessionTable[clientSocket].addr);
							PushThreadJob(new MainJob(CMD_CONNECT, clientSocket, addrText));
							Logger::Log("[SESSION IN] " + sessionTable[clientSocket].addr);
						}
					}
					// Recv
					else
					{
						int recvLength = recv(readySoc, reinterpret_cast<char*>(buffer), BUF_SIZE, 0);
						if (recvLength <= 0) //����ó��
						{
							//�������Ḧ WorkerThread�� �뺸
							if (0 == recvLength)
							{
								PushThreadJob(new MainJob(CMD_DECREASE, readySoc, nullptr));
							}
							else if (0 > recvLength) //�����ڵ� �ڵ鸵
							{
								PushThreadJob(new MainJob(CMD_DECREASE, readySoc, nullptr));
							}

							//select ���̺��� ����
							Logger::Log("[SESSION OUT] " + sessionTable[readySoc].addr);
							FD_CLR(readySoc, &masterFdSet);
							sessionTable.erase(readySoc);
							continue;
						}

						//�־��� ���ۿ��� ���������� ������ ����
						std::string& userBuffer = sessionTable[readySoc].buffer;
						size_t prevPos;
						if (false == userBuffer.empty())
						{
							prevPos = userBuffer.size() - 1;
						}
						else
						{
							prevPos = 0;
						}
						for (int i = 0; i < recvLength; ++i)
						{
							if (buffer[i] == VK_BACK) //�齺���̽��� ���� �����Ϳ��� �� ���ڸ� ����.
								userBuffer.pop_back();
							else
								userBuffer.push_back(buffer[i]);
						}
						while (true)
						{
							size_t cmdPos = userBuffer.find("\r\n", prevPos); // ���๮�� �߰� �� ��Ŷ ó��
							if (std::string::npos != cmdPos)
							{
								if (0 != cmdPos) // ���͸� ��Ÿ�� ġ�� ���� ó������ �ʴ´�.
								{
									PushThreadJob(new MainJob(CMD_PROCESS, readySoc, new std::string(userBuffer.substr(0, cmdPos))));
								}
								userBuffer = userBuffer.substr(cmdPos + 2);
								prevPos = userBuffer.size() - 1;
							}
							else break;
						}
						//�����ϰ� �޸𸮸� �����ϴ� ���� ���� ���� ������ �� �ִ� �ִ� ������ ũ�� ����.
						if (userBuffer.size() > USERBUF_SIZE)
						{
							userBuffer.clear();
						}
					}
				}
			}
		}
	}
}

void ChatServer::Terminate()
{
	//Listen ���� ���� �� WSA ����.
	closesocket(m_listener);
	WSACleanup();
	Logger::Log("[Terminate Server]");
}

bool ChatServer::InitWSA(short port)
{
	Logger::Log("[Initializing ChatServer - PORT " + std::to_string(port) + "]");
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
	if (LOBBY_INDEX != g_roomManager.CreateRoom("Lobby", MAX_LOBBY_SIZE, false))
	{
		return false;
	}
	return true;
}

void ChatServer::ProcessPacket(const UserJob* jobPtr)
{
	static std::string alreadyLoginMsg{ "[����]�̹� �α��εǾ��ֽ��ϴ�." };
	static std::string plzLoginMsg{ "[����]���� ���� " + std::to_string(MAX_IDLENGTH) + "����Ʈ ���� ���̵�� �α����� ���ּ���.\r\n/login [ID]" };
	std::string welcomeMsg{ "=====================\r\nWelcome To ChatServer\r\n=====================\r\n���� ���� " + std::to_string(MAX_IDLENGTH) + "����Ʈ ���� ���̵�� �α����� ���ּ���.\r\n/login [ID]" };

	if (nullptr != jobPtr) {
		if (jobPtr->cmd == CMD_CONNECT)
		{
			g_userManager.AddUser(jobPtr->socket, jobPtr->data[0]);
			g_userManager.SendMsg(jobPtr->socket, welcomeMsg);

			return;
		}
		if (jobPtr->cmd == CMD_DECREASE)
		{
			g_userManager.DecreaseUser(jobPtr->socket);
			return;
		}

		User* user = g_userManager.GetUser(jobPtr->socket);
		if (nullptr != user)
		{
			const std::vector<std::string>& data = jobPtr->data;
			//�α���, ��ɾ� ���ο� ���� ���� ó�� �б�
			if (true == user->m_login)
			{
				switch (jobPtr->cmd)
				{
				case CMD_HELP:
					ProcessHelp(user);
					break;

				case CMD_LOGIN:
					user->SendChat(alreadyLoginMsg);
					break;

				case CMD_CHAT:
					ProcessChat(user, data[0]);
					break;

				case CMD_JOIN:
					ProcessJoin(user, std::stoi(data[1]));
					break;

				case CMD_QUIT:
					ProcessQuit(user);
					break;

				case CMD_MSG:
					ProcessMsg(user, data[1], data[2]);
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
					ProcessCreateRoom(user, data[1], std::stoi(data[2]));
					break;

				case CMD_ERROR:
					ProcessError(user);
					break;
				}
			}
			else
			{
				//�α����� �ȵ� ��� �α��θ� ó��.
				switch (jobPtr->cmd)
				{
				case CMD_LOGIN:
					ProcessLogin(user, data[1]);
					break;

				default:
					user->SendChat(plzLoginMsg);
					break;
				}
			}
		}
	}
}

void ChatServer::ProcessLogin(User* user, const std::string& userName)
{
	static std::string errMsg{ "[����]ID�� �ߺ��˴ϴ�." };
	static std::string longIdMsg{ "[����]ID�� " + std::to_string(MAX_IDLENGTH) + "����Ʈ ���Ͽ��� �մϴ�." };

	if (nullptr != user)
	{
		if (userName.size() > MAX_IDLENGTH)
		{
			user->SendChat(longIdMsg);
			return;
		}
		if (false == user->m_login)
		{
			//���� ���̺� �߰��� �� ���� �޼��� ���, �κ� ����.
			if (true == g_userManager.Login(user->m_socket, userName))
			{
				Logger::Log("[USER LOGIN] " + user->m_addr + " " + user->m_name);
				g_roomManager.Enter(user->m_socket, user->m_name, LOBBY_INDEX);
				user->m_room = LOBBY_INDEX;
				ProcessHelp(user);
				return;
			}
			user->SendChat(errMsg);
		}
		return;
	}
}

void ChatServer::ProcessChat(User* sender, const std::string& msg)
{
	if (nullptr != sender)
	{
		const Room* room = g_roomManager.GetRoom(sender->m_room);
		if (nullptr != room)
		{
			room->SendChat(sender->m_socket, msg);
		}
	}
}

void ChatServer::ProcessJoin(User* user, int roomIdx)
{
	static std::string manyPeopleMsg{ "[����]�ο��� �ʰ��� ������ �� �����ϴ�." };
	static std::string alreadyExistMsg{ "[����]���� �ִ� ���Դϴ�." };
	static std::string noExistMsg{ "[����]���� ���ȣ�Դϴ�." };

	if (nullptr != user)
	{
		//���� �濡�� ������ �Ұ��ϴ�.
		if (user->m_room == roomIdx)
		{
			user->SendChat(alreadyExistMsg);
			return;
		}

		const Room* newRoom = g_roomManager.GetRoom(roomIdx);
		if (nullptr == newRoom)
		{
			//�ε����� �ش��ϴ� ���� ���ٸ� ���� �޼��� ����.
			user->SendChat(noExistMsg);
			return;
		}
		else
		{
			if (true == g_roomManager.Enter(user->m_socket, user->m_name, roomIdx))
			{
				g_roomManager.Leave(user->m_socket, user->m_room);
				user->m_room = roomIdx;
			}
			else
			{
				user->SendChat(manyPeopleMsg);
			}
		}
	}
}

void ChatServer::ProcessQuit(User* user)
{
	static std::string errMsg{ "[����]�κ񿡼��� ������ �� �����ϴ�." };
	if (nullptr != user)
	{
		//���� �濡�� ������ �Ұ��ϴ�.
		if (user->m_room == LOBBY_INDEX)
		{
			user->SendChat(errMsg);
			return;
		}

		if (true == g_roomManager.Enter(user->m_socket, user->m_name, LOBBY_INDEX))
		{
			g_roomManager.Leave(user->m_socket, user->m_room);
			user->m_room = LOBBY_INDEX;
		}
	}
}

void ChatServer::ProcessMsg(User* sender, const std::string& receiverName, const std::string& msg)
{
	static std::string cantSendSamePeopleMsg{ "[����]���ο��Դ� ������ �� �����ϴ�." };
	static std::string cantFindPeopleMsg{ "[����]������ ã�� �� �����ϴ�." };
	static std::string noMsg{ "[����]���� �޼����� ������ �� �����ϴ�." };

	if (nullptr != sender)
	{
		// ���ο��Դ� ������ �� ����.
		if (sender->m_name == (receiverName))
		{
			sender->SendChat(cantSendSamePeopleMsg);
			return;
		}

		// �ش� �̸��� ���� ������ Ž��
		User* receiver = g_userManager.GetUser(receiverName);
		if (nullptr == receiver)
		{
			sender->SendChat(cantFindPeopleMsg);
			return;
		}

		//CmdParser�� ������ msg�� �� ���� �̻� ������ �����Ѵ�.
		if (' ' == msg.back())
		{
			//���ڿ� �� �� ���鹮�ڸ� �����ؼ�
			std::string noBackWhiteSpaceMsg = msg;
			m_cmdParser.EraseBackWhiteSpace(noBackWhiteSpaceMsg);
			//���鹮�ڿ��̸� ����ó��
			if (true == noBackWhiteSpaceMsg.empty())
			{
				sender->SendChat(noMsg);
				return;
			}
			//�ƴϸ� ��������
			receiver->SendChat("[MESSAGE FROM] " + sender->m_name + " " + noBackWhiteSpaceMsg);
			return;
		}
		else
		{
			receiver->SendChat("[MESSAGE FROM] " + sender->m_name + " " + msg);
		}
	}
}

void ChatServer::ProcessGetUserList(User* user)
{
	if (nullptr != user)
	{
		//������ �ִ� ���� ���� ��� ȹ�� �� ����
		Room* userRoom = g_roomManager.GetRoom(user->m_room);
		if (nullptr != userRoom)
		{
			std::string userList = userRoom->GetUserList();
			user->SendChat(userList);
		}
	}
}

void ChatServer::ProcessGetRoomList(User* user)
{
	if (nullptr != user)
	{
		//roomManager�� ������ �� ��� ȹ�� �� ����.
		user->SendChat(g_roomManager.GetRoomList());
	}
}

void ChatServer::ProcessGetAllUserList(User* user)
{
	if (nullptr != user)
	{
		//������ ��� ������ ��� ȹ�� �� ����.
		user->SendChat(g_userManager.GetUserList());
	}
}

void ChatServer::ProcessCreateRoom(User* user, const std::string& roomName, int maxUser)
{
	static std::string errMsg{ "[����]���� �ִ��ο� ���� " + std::to_string(MINUSER_NUM) + " �̻�, " + std::to_string(MAXUSER_NUM) + " �����̾�� �մϴ�." };
	static std::string failMakeRoomMsg{ "[����]�� ������ �����Ͽ����ϴ�." };
	if (nullptr != user)
	{
		if (MINUSER_NUM > maxUser ||
			MAXUSER_NUM < maxUser
			)
		{
			user->SendChat(errMsg);
			return;
		}
		//�� ������ ���� ����
		int newRoom = g_roomManager.CreateRoom(roomName, maxUser);
		if (OUT_OF_RANGE == newRoom)
		{
			user->SendChat(errMsg);
			return;
		}

		if (true == g_roomManager.Enter(user->m_socket, user->m_name, newRoom))
		{
			g_roomManager.Leave(user->m_socket, user->m_room);
			user->m_room = newRoom;
		}
	}
}

void ChatServer::ProcessHelp(User* user)
{
	//���� �޼��� ����
	static std::string helpCmd{
"[����]\r\n\
[����] /help\r\n\
[����] /join [���ȣ]\r\n\
[����] /quit\r\n\
[����] /msg [����] [�޼���]\r\n\
[�� ����] /create [���̸�] [�ִ��ο�]\r\n\
[���� �� ���� ���] /alluserlist\r\n" };

	/*static std::string helpCmd{
"\r\n[����]\r\n\
[����] /help\r\n\
[����] /join [���ȣ]\r\n\
[����] /quit\r\n\
[����] /msg [����] [�޼���]\r\n\
[�� ����] /create [���̸�] [�ִ��ο�]\r\n\
[�� ���] /roomlist\r\n\
[�� �� ���� ���] /userlist\r\n\
[���� �� ���� ���] /alluserlist\r\n" };*/

	user->SendChat(helpCmd);
}

void ChatServer::ProcessError(User* user)
{
	if (nullptr != user)
	{
		//��ɾ� ��Ͽ� ���� ��ɾ ���� �� ó��.
		static std::string wrongCmd{ " �߸��� ��ɾ� �����Դϴ�." };
		user->SendChat(wrongCmd);
	}
}


void ChatServer::InitMultiThread()
{
	for (int i = 0; i < WORKERTHREAD_NUM; ++i)
	{
		m_workerThreads.emplace_back(&ChatServer::WorkerThread, this);
	}
}

void ChatServer::WorkerThread()
{
	std::mutex Locker;
	thread_id = abc++;
	MainJob* curJob = nullptr;
	while (true)
	{
		std::unique_lock<std::mutex> uLock(Locker);
		m_notifier.wait(uLock, [&] { return 0 < m_workerJobCnt; });

		do {
			int remainJob = m_workerJobCnt;
			if (0 < remainJob)
			{
				if (true == std::atomic_compare_exchange_strong(&m_workerJobCnt, &remainJob, 0))
				{
					for (int left = 0; left < remainJob; ++left)
					{
						while (false == m_workerQueue.try_pop(curJob)) {};

						if (nullptr != curJob)
						{
							//Logger::Log("[USER SEND] " + *curJob->data);
							switch (curJob->cmd)
							{
							case CMD_CONNECT:
								PushUserJob(new UserJob(CMD_CONNECT, curJob->socket, 0, curJob->data));
								break;
							case CMD_DECREASE:
								PushUserJob(new UserJob(CMD_DECREASE, curJob->socket, 0, 0));
								break;

							case CMD_PROCESS:
							{
								//��ɾ �Ľ� ��
								std::smatch param;
								int cmd = m_cmdParser.Parse(*curJob->data, param);
								if (false == param.empty())
								{
									PushUserJob(new UserJob(cmd, curJob->socket, 0, param));
								}
								else
								{
									PushUserJob(new UserJob(cmd, curJob->socket, 0, curJob->data));
								}
							}
							break;

							case CMD_SEND:
								if (INVALID_SOCKET != curJob->socket)
								{
									int sendLength = send(curJob->socket, curJob->data->c_str(), static_cast<int>(curJob->data->size()), 0);
									if (sendLength == 0) //����ó��
									{
										PushUserJob(new UserJob(CMD_DECREASE, curJob->socket, 0, curJob->data));
									}
									else if (sendLength < 0)
									{
										PushUserJob(new UserJob(CMD_DECREASE, curJob->socket, 0, curJob->data));
									}
									PushUserJob(new UserJob(CMD_DECREASE, curJob->socket, 0, curJob->data));
								}
								break;
							}
							delete curJob;
							curJob = nullptr;
						}
					}
				}
			}
		} while (0 < m_workerJobCnt);
	}
}

void ChatServer::RecvThread(SessionTable* sessions)
{
	if (nullptr != sessions)
	{
		//Folk Thread�� Recv�� ó���Ѵ�.
		Logger::Log("[RECV THREAD FOLK]");
		SessionTable& sessionTable = *sessions;

		fd_set copyFdSet;
		fd_set masterFdSet;

		SOCKET maxFd = 0;
		FD_ZERO(&masterFdSet);
		for (const auto& session : sessionTable)
		{
			FD_SET(session.first, &masterFdSet);
			maxFd = max(session.first, maxFd);
		}

		char buffer[BUF_SIZE + 1] = { 0, };
		while (SOCKET_LOWER_BOUND < sessionTable.size())
		{
			copyFdSet = masterFdSet;
			int numFd = select(static_cast<int>(maxFd) + 1, &copyFdSet, 0, 0, 0);
			assert(numFd >= 0);
			if (numFd != 0)
			{
				for (int readySoc = 0; readySoc < maxFd + 1; ++readySoc)
				{
					if (FD_ISSET(readySoc, &copyFdSet))
					{
						int recvLength = recv(readySoc, reinterpret_cast<char*>(buffer), BUF_SIZE, 0);
						if (recvLength <= 0) //����ó��
						{
							//�������Ḧ WorkerThread�� �뺸
							if (0 == recvLength)
							{
								PushThreadJob(new MainJob(CMD_DECREASE, readySoc, nullptr));
							}
							else if (0 > recvLength) //�����ڵ� �ڵ鸵
							{
								PushThreadJob(new MainJob(CMD_DECREASE, readySoc, nullptr));
							}

							//select ���̺��� ����
							Logger::Log("[SESSION OUT] " + sessionTable[readySoc].addr);
							FD_CLR(readySoc, &masterFdSet);
							sessionTable.erase(readySoc);
							continue;
						}

						//�־��� ���ۿ��� ���������� ������ ����
						std::string& userBuffer = sessionTable[readySoc].buffer;
						size_t prevPos;
						if (false == userBuffer.empty())
						{
							prevPos = userBuffer.size() - 1;
						}
						else
						{
							prevPos = 0;
						}
						for (int i = 0; i < recvLength; ++i)
						{
							if (buffer[i] == VK_BACK) //�齺���̽��� ���� �����Ϳ��� �� ���ڸ� ����.
								userBuffer.pop_back();
							else
								userBuffer.push_back(buffer[i]);
						}
						while (true)
						{
							size_t cmdPos = userBuffer.find("\r\n", prevPos); // ���๮�� �߰� �� ��Ŷ ó��
							if (std::string::npos != cmdPos)
							{
								if (0 != cmdPos) // ���͸� ��Ÿ�� ġ�� ���� ó������ �ʴ´�.
								{
									PushThreadJob(new MainJob(CMD_PROCESS, readySoc, new std::string(userBuffer.substr(0, cmdPos))));
								}
								userBuffer = userBuffer.substr(cmdPos + 2);
								prevPos = userBuffer.size() - 1;
							}
							else break;
						}
						//�����ϰ� �޸𸮸� �����ϴ� ���� ���� ���� ������ �� �ִ� �ִ� ������ ũ�� ����.
						if (userBuffer.size() > USERBUF_SIZE)
						{
							userBuffer.clear();
						}
					}
				}
			}
		}

		//������ ���� ���� ���Ϸ� �����ϸ�
		//���ν������ ���� �� �����带 �����Ѵ�.
		m_sessionQueue.push(sessions);
		Logger::Log("[RECV THREAD END]");
	}
}

void ChatServer::PushThreadJob(MainJob* jobPtr)
{
	if (nullptr != jobPtr)
	{
		++m_workerJobCnt;
		m_workerQueue.push(jobPtr);
		m_notifier.notify_one();
	}
}

void ChatServer::PushUserJob(UserJob* jobPtr)
{
	//Logger::Log("ENQUE " + std::to_string(jobPtr->cmd));
	if (nullptr != jobPtr)
	{
		if (0 != m_userJobCnt.fetch_add(1))
		{
			m_userQueue.push(jobPtr);
		}
		else
		{
			m_userQueue.push(jobPtr);
			int remainJob = m_userJobCnt;
			int left;
			UserJob* newJob = nullptr;
			//Logger::Log("		Flush Start " + std::to_string(thread_id));
			do {
				for (left = 0; left < remainJob; ++left)
				{
					while (false == m_userQueue.try_pop(newJob)) {};
					ProcessPacket(newJob);
					delete newJob;
					newJob = nullptr;
				}
			} while (remainJob != m_userJobCnt.fetch_sub(remainJob));
			//Logger::Log("		Flush End " + std::to_string(thread_id));
		}
	}
}