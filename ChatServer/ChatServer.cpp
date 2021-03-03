#include "ChatServer.h"
thread_local int thread_id;
std::atomic_int abc;

#define LOGIN_ON
ChatServer& ChatServer::Instance()
{
	//싱글턴 객체 생성 및 호출.
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
	std::string bufferoverMsg{ std::to_string(USERBUF_SIZE) + "자 이상으로 문자를 입력할 수 없습니다." };
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
		// 합칠 recvThread가 있으면 메인 스레드와 합침.
		while (false == m_sessionQueue.empty())
		{
			SessionTable* stocks = nullptr;
			while (false == m_sessionQueue.try_pop(stocks)) {};
			for (const auto& session : *stocks)
			{
				sessionTable[session.first] = session.second;
			}
			delete stocks;

			//다시 재설정.
			FD_ZERO(&masterFdSet);
			FD_SET(m_listener, &masterFdSet);
			maxFd = m_listener;
			for (const auto& session : sessionTable)
			{
				FD_SET(session.first, &masterFdSet);
				maxFd = max(session.first, maxFd);
			}
		}

		// -1은 ListenSocket을 고려한 것.
		// 63개 단위로 recvThread folk.
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
			//다시 재설정.
			FD_ZERO(&masterFdSet);
			FD_SET(m_listener, &masterFdSet);
			maxFd = m_listener;
			for (const auto& session : sessionTable)
			{
				FD_SET(session.first, &masterFdSet);
				maxFd = max(session.first, maxFd);
			}
		}

		//실질 처리부.
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
							// recv용 소켓컨테이너에 등록
							sessionTable.emplace(clientSocket, Session());
							// 세션에 유저 추가
							sessionTable[clientSocket].socket = clientSocket;
							sessionTable[clientSocket].addr.clear();
							sessionTable[clientSocket].buffer.clear();

							// 유저 접속 이벤트 포스팅
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
						if (recvLength <= 0) //종료처리
						{
							//접속종료를 WorkerThread에 통보
							if (0 == recvLength)
							{
								PushThreadJob(new MainJob(CMD_DECREASE, readySoc, nullptr));
							}
							else if (0 > recvLength) //에러코드 핸들링
							{
								PushThreadJob(new MainJob(CMD_DECREASE, readySoc, nullptr));
							}

							//select 테이블에서 삭제
							Logger::Log("[SESSION OUT] " + sessionTable[readySoc].addr);
							FD_CLR(readySoc, &masterFdSet);
							sessionTable.erase(readySoc);
							continue;
						}

						//주어진 버퍼에서 순차적으로 데이터 삽입
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
							if (buffer[i] == VK_BACK) //백스페이스는 기존 데이터에서 한 글자를 뺀다.
								userBuffer.pop_back();
							else
								userBuffer.push_back(buffer[i]);
						}
						while (true)
						{
							size_t cmdPos = userBuffer.find("\r\n", prevPos); // 개행문자 발견 시 패킷 처리
							if (std::string::npos != cmdPos)
							{
								if (0 != cmdPos) // 엔터만 연타로 치는 경우는 처리하지 않는다.
								{
									PushThreadJob(new MainJob(CMD_PROCESS, readySoc, new std::string(userBuffer.substr(0, cmdPos))));
								}
								userBuffer = userBuffer.substr(cmdPos + 2);
								prevPos = userBuffer.size() - 1;
							}
							else break;
						}
						//과다하게 메모리를 차지하는 것을 막기 위해 저장할 수 있는 최대 데이터 크기 제한.
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
	//Listen 소켓 종료 후 WSA 종료.
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
	// 로비 생성, 로비는 사실상 인원제한이 없다.
	if (LOBBY_INDEX != g_roomManager.CreateRoom("Lobby", MAX_LOBBY_SIZE, false))
	{
		return false;
	}
	return true;
}

void ChatServer::ProcessPacket(const UserJob* jobPtr)
{
	static std::string alreadyLoginMsg{ "[에러]이미 로그인되어있습니다." };
	static std::string plzLoginMsg{ "[에러]공백 없이 " + std::to_string(MAX_IDLENGTH) + "바이트 이하 아이디로 로그인을 해주세요.\r\n/login [ID]" };
	std::string welcomeMsg{ "=====================\r\nWelcome To ChatServer\r\n=====================\r\n공백 없이 " + std::to_string(MAX_IDLENGTH) + "바이트 이하 아이디로 로그인을 해주세요.\r\n/login [ID]" };

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
			//로그인, 명령어 여부에 따라 이후 처리 분기
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
				//로그인이 안된 경우 로그인만 처리.
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
	static std::string errMsg{ "[에러]ID가 중복됩니다." };
	static std::string longIdMsg{ "[에러]ID는 " + std::to_string(MAX_IDLENGTH) + "바이트 이하여야 합니다." };

	if (nullptr != user)
	{
		if (userName.size() > MAX_IDLENGTH)
		{
			user->SendChat(longIdMsg);
			return;
		}
		if (false == user->m_login)
		{
			//유저 테이블에 추가한 후 도움말 메세지 출력, 로비에 입장.
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
	static std::string manyPeopleMsg{ "[에러]인원수 초과로 입장할 수 없습니다." };
	static std::string alreadyExistMsg{ "[에러]현재 있는 방입니다." };
	static std::string noExistMsg{ "[에러]없는 방번호입니다." };

	if (nullptr != user)
	{
		//같은 방에는 입장이 불가하다.
		if (user->m_room == roomIdx)
		{
			user->SendChat(alreadyExistMsg);
			return;
		}

		const Room* newRoom = g_roomManager.GetRoom(roomIdx);
		if (nullptr == newRoom)
		{
			//인덱스에 해당하는 방이 없다면 오류 메세지 전송.
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
	static std::string errMsg{ "[에러]로비에서는 나가실 수 없습니다." };
	if (nullptr != user)
	{
		//같은 방에는 입장이 불가하다.
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
	static std::string cantSendSamePeopleMsg{ "[에러]본인에게는 전송할 수 없습니다." };
	static std::string cantFindPeopleMsg{ "[에러]유저를 찾을 수 없습니다." };
	static std::string noMsg{ "[에러]공백 메세지는 전송할 수 없습니다." };

	if (nullptr != sender)
	{
		// 본인에게는 전송할 수 없다.
		if (sender->m_name == (receiverName))
		{
			sender->SendChat(cantSendSamePeopleMsg);
			return;
		}

		// 해당 이름을 가진 유저를 탐색
		User* receiver = g_userManager.GetUser(receiverName);
		if (nullptr == receiver)
		{
			sender->SendChat(cantFindPeopleMsg);
			return;
		}

		//CmdParser는 무조건 msg가 한 글자 이상 들어옴을 보장한다.
		if (' ' == msg.back())
		{
			//문자열 맨 뒤 공백문자를 제거해서
			std::string noBackWhiteSpaceMsg = msg;
			m_cmdParser.EraseBackWhiteSpace(noBackWhiteSpaceMsg);
			//공백문자열이면 에러처리
			if (true == noBackWhiteSpaceMsg.empty())
			{
				sender->SendChat(noMsg);
				return;
			}
			//아니면 정상전송
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
		//유저가 있는 방의 유저 목록 획득 후 전송
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
		//roomManager에 생성된 방 목록 획득 후 전송.
		user->SendChat(g_roomManager.GetRoomList());
	}
}

void ChatServer::ProcessGetAllUserList(User* user)
{
	if (nullptr != user)
	{
		//접속한 모든 유저의 목록 획득 후 전송.
		user->SendChat(g_userManager.GetUserList());
	}
}

void ChatServer::ProcessCreateRoom(User* user, const std::string& roomName, int maxUser)
{
	static std::string errMsg{ "[에러]방의 최대인원 수는 " + std::to_string(MINUSER_NUM) + " 이상, " + std::to_string(MAXUSER_NUM) + " 이하이어야 합니다." };
	static std::string failMakeRoomMsg{ "[에러]방 생성에 실패하였습니다." };
	if (nullptr != user)
	{
		if (MINUSER_NUM > maxUser ||
			MAXUSER_NUM < maxUser
			)
		{
			user->SendChat(errMsg);
			return;
		}
		//방 생성후 유저 입장
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
	//도움말 메세지 전송
	static std::string helpCmd{
"[도움말]\r\n\
[도움] /help\r\n\
[입장] /join [방번호]\r\n\
[퇴장] /quit\r\n\
[쪽지] /msg [상대방] [메세지]\r\n\
[방 생성] /create [방이름] [최대인원]\r\n\
[서버 내 유저 목록] /alluserlist\r\n" };

	/*static std::string helpCmd{
"\r\n[도움말]\r\n\
[도움] /help\r\n\
[입장] /join [방번호]\r\n\
[퇴장] /quit\r\n\
[쪽지] /msg [상대방] [메세지]\r\n\
[방 생성] /create [방이름] [최대인원]\r\n\
[방 목록] /roomlist\r\n\
[방 내 유저 목록] /userlist\r\n\
[서버 내 유저 목록] /alluserlist\r\n" };*/

	user->SendChat(helpCmd);
}

void ChatServer::ProcessError(User* user)
{
	if (nullptr != user)
	{
		//명령어 목록에 없는 명령어가 왔을 시 처리.
		static std::string wrongCmd{ " 잘못된 명령어 형식입니다." };
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
								//명령어를 파싱 후
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
									if (sendLength == 0) //종료처리
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
		//Folk Thread는 Recv만 처리한다.
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
						if (recvLength <= 0) //종료처리
						{
							//접속종료를 WorkerThread에 통보
							if (0 == recvLength)
							{
								PushThreadJob(new MainJob(CMD_DECREASE, readySoc, nullptr));
							}
							else if (0 > recvLength) //에러코드 핸들링
							{
								PushThreadJob(new MainJob(CMD_DECREASE, readySoc, nullptr));
							}

							//select 테이블에서 삭제
							Logger::Log("[SESSION OUT] " + sessionTable[readySoc].addr);
							FD_CLR(readySoc, &masterFdSet);
							sessionTable.erase(readySoc);
							continue;
						}

						//주어진 버퍼에서 순차적으로 데이터 삽입
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
							if (buffer[i] == VK_BACK) //백스페이스는 기존 데이터에서 한 글자를 뺀다.
								userBuffer.pop_back();
							else
								userBuffer.push_back(buffer[i]);
						}
						while (true)
						{
							size_t cmdPos = userBuffer.find("\r\n", prevPos); // 개행문자 발견 시 패킷 처리
							if (std::string::npos != cmdPos)
							{
								if (0 != cmdPos) // 엔터만 연타로 치는 경우는 처리하지 않는다.
								{
									PushThreadJob(new MainJob(CMD_PROCESS, readySoc, new std::string(userBuffer.substr(0, cmdPos))));
								}
								userBuffer = userBuffer.substr(cmdPos + 2);
								prevPos = userBuffer.size() - 1;
							}
							else break;
						}
						//과다하게 메모리를 차지하는 것을 막기 위해 저장할 수 있는 최대 데이터 크기 제한.
						if (userBuffer.size() > USERBUF_SIZE)
						{
							userBuffer.clear();
						}
					}
				}
			}
		}

		//세션이 일정 갯수 이하로 감소하면
		//메인스레드와 병합 후 스레드를 종료한다.
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