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
	std::string bufferoverMsg{ std::to_string(USERBUF_SIZE) + "자 이상으로 문자를 입력할 수 없습니다." };
	std::string welcomeMsg{ "=====================\r\nWelcome To ChatServer\r\n=====================\r\n공백 없이 " + std::to_string(MAX_IDLENGTH) + "바이트 이하 아이디로 로그인을 해주세요.\r\n/login [ID]" };
	Logger::Log("[Start Running]");

	// Recv는 순차적으로 처리된다.
	// Accept, Disconnect로 발생한 소켓 변경은 동기적으로 처리가능하다.
	// => Recv용 소켓은 지역변수로 관리 가능.
	std::deque<SOCKET> recvSockets; // 순회가 잦으면서도 추가삭제가 반복적으로 일어나 deque 선택.
	recvSockets.emplace_back(m_listener);
	std::vector<fd_set> masterFdSets;
	std::vector<SOCKET> maxFds;
	bool dirtyFlag = true;

	masterFdSets.emplace_back();
	maxFds.emplace_back(0);

	//select는 64인 제한이 있음.
	//이를 해결하기 위해 소켓을 순차적으로 fd_set에 추가, select를 반복적으로 호출한다.
	//매번 fd_set을 갱신하는 것은 오버헤드가 있으므로
	//갱신이 필요한 상황(Accept, Disconnect)에서만 dirtyFlag를 세팅, 다음 순회에서 갱신한다.
	timeval timeout{ 0, 0 };
	fd_set copyFdSet;
	char buffer[BUF_SIZE + 1];
	while (true)
	{
		int loopCnt = (static_cast<int>(recvSockets.size()) - 1) / FD_SETSIZE + 1;
		// 소켓 리스트에 변경 발생 시 FdSet 갱신.
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

		// 이후 반복적으로 select 호출.
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
							// recv용 소켓컨테이너에 등록
							recvSockets.emplace_back(clientSocket);
							dirtyFlag = true;

							UserPtr newUser = AddSession(clientSocket, clientAddr);
							assert(nullptr != newUser); // 세션 생성 실패 - map인데 실패한다? 문제있음

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

						if (recvLength <= 0) //종료처리
						{
							g_userManager.DisconnectUser(user); //유저테이블 삭제

							//select 테이블에서 삭제
							auto delPos = std::find(recvSockets.begin(), recvSockets.end(), readySoc);
							assert(recvSockets.end() != delPos);
							recvSockets.erase(delPos);
							dirtyFlag = true;

							//세션 테이블 삭제
							Logger::Log("[SESSION Out] " + user->GetAddr());
							assert(1 == EraseSession(readySoc));

							if (recvLength < 0) //에러코드 핸들링
							{
								Logger::WsaLog(user->GetAddr().c_str(), WSAGetLastError());
							}
							continue;
						}

						int prevPos = max(0, static_cast<int>(user->m_data.size()) - 1);
						user->PushData(buffer, recvLength);
						while (true)
						{
							size_t cmdPos = user->m_data.find("\r\n", prevPos); // 개행문자 발견 시 패킷 처리
							if (std::string::npos != cmdPos)
							{
								if (0 != cmdPos) // 엔터만 연타로 치는 경우는 처리하지 않는다.
								{
									ProcessPacket(user, user->m_data.substr(0, cmdPos));
								}
								user->m_data = user->m_data.substr(cmdPos + 2);
							}
							else break;
						}
						//과다하게 메모리를 차지하는 것을 막기 위해 저장할 수 있는 최대 데이터 크기 제한.
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
	//Listen 소켓 종료 후 WSA 종료.
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
	// 로비 생성, 로비는 사실상 인원제한이 없다.
	m_lobby = g_roomManager.CreateRoom("Lobby", MAX_LOBBY_SIZE, false);
	if (nullptr == m_lobby)
		return false;
	m_lobby->SetWeakPtr(m_lobby);
	return true;
}

void ChatServer::ProcessPacket(UserPtr& user, std::string data)
{
	static std::string alreadyLoginMsg{ "이미 로그인되어있습니다." };
	static std::string plzLoginMsg{ "공백 없이 " + std::to_string(MAX_IDLENGTH) + "바이트 이하 아이디로 로그인을 해주세요.\r\n/login [ID]" };

	if (nullptr == user)
	{
		return;
	}

	Logger::Log("[USER SEND] " + user->GetAddr() + " " + user->GetName() + " " + data);

	//명령어를 파싱 후
	std::smatch param;
	int cmd = m_cmdParser.Parse(data, param);

	//로그인, 명령어 여부에 따라 이후 처리 분기
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
		//로그인이 안된 경우 로그인만 처리.
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
	static std::string manyPeopleMsg{ "인원수 초과로 입장할 수 없습니다." };
	if (nullptr == user ||
		nullptr == enterRoom)
	{
		return;
	}

	//새로운 방에 입장 후 이전 방을 나가는 방식으로 작동한다.
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
		//새로운 방에 입장 실패 시 오류 메세지 전송
		user->SendChat(manyPeopleMsg);
	}
}

UserPtr ChatServer::AddSession(SOCKET socket, SOCKADDR_IN addr)
{
	//Accept 발생 시 새로운 세션을 생성한다.
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
	static std::string errMsg{ "[로그인 실패] ID가 중복됩니다." };
	static std::string longIdMsg{ "[로그인 실패] ID는 " + std::to_string(MAX_IDLENGTH) + "바이트 이하여야 합니다." };

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
		//유저 테이블에 추가한 후 도움말 메세지 출력, 로비에 입장.
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
	static std::string alreadyExistMsg{ "현재 있는 방입니다." };
	static std::string noExistMsg{ "없는 방번호입니다." };

	if (nullptr == user)
	{
		return;
	}

	RoomPtr oldRoom = user->GetRoom();
	if (nullptr != oldRoom)
	{
		//같은 방으로의 재입장은 차단.
		if (true == oldRoom->IsSameIdx(roomIdx))
		{
			user->SendChat(alreadyExistMsg);
			return;
		}
	}


	RoomPtr newRoom = g_roomManager.GetRoom(roomIdx);
	if (nullptr == newRoom)
	{
		//인덱스에 해당하는 방이 없다면 오류 메세지 전송.
		user->SendChat(noExistMsg);
		return;
	}
	ExchangeRoom(user, newRoom);
}

void ChatServer::ProcessQuit(UserPtr &user)
{
	static std::string errMsg{ "로비에서는 나가실 수 없습니다." };
	if (nullptr == user)
	{
		return;
	}

	RoomPtr userRoom = user->GetRoom();
	if (nullptr == userRoom)
	{
		return;
	}

	//로비를 제외한 곳에서만 퇴장 가능.
	//기존방 -> 로비로의 이동을 처리.
	if (m_lobby == userRoom)
	{
		user->SendChat(errMsg);
		return;
	}
	ExchangeRoom(user, m_lobby);
}

void ChatServer::ProcessMsg(const UserPtr& sender, const std::string& receiverName, const std::string& msg)
{
	static std::string cantSendSamePeopleMsg{ "본인에게는 전송할 수 없습니다." };
	static std::string cantFindPeopleMsg{ "유저를 찾을 수 없습니다." };
	if (nullptr == sender)
	{
		return;
	}

	// 본인에게는 전송할 수 없다.
	if (sender->GetName() == ("[" + receiverName + "]"))
	{
		sender->SendChat(cantSendSamePeopleMsg);
		return;
	}

	// 해당 이름을 가진 유저를 탐색
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

	//유저가 있는 방의 유저 목록 획득 후 전송
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

	//roomManager에 생성된 방 목록 획득 후 전송.
	std::string roomList = g_roomManager.GetRoomList();
	user->SendChat(roomList);
}

void ChatServer::ProcessGetAllUserList(const UserPtr & user)
{
	if (nullptr == user)
	{
		return;
	}
	//접속한 모든 유저의 목록 획득 후 전송.
	user->SendChat(g_userManager.GetUserList());
}

void ChatServer::ProcessCreateRoom(UserPtr & user, const std::string& roomName, int maxUser)
{
	static std::string errMsg{ "방의 최대인원 수는 " + std::to_string(MINUSER_NUM) + " 이상, " + std::to_string(MAXUSER_NUM) + " 이하이어야 합니다." };
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
	//방 생성후 유저 입장
	RoomPtr newRoom = g_roomManager.CreateRoom(roomName, maxUser);
	if (nullptr == newRoom)
	{
		Logger::Log("[Error] - 방 생성 실패");
		user->SendChat("방 생성에 실패하였습니다.");
		return;
	}
	ExchangeRoom(user, newRoom);

}

void ChatServer::ProcessHelp(const UserPtr & user)
{
	//도움말 메세지 전송
	static std::string helpCmd{
"\r\n== 명령어 목록 == \r\n\
[도움] /help\r\n\
[입장] /join [방번호]\r\n\
[퇴장] /quit\r\n\
[쪽지] /msg [상대방] [메세지]\r\n\
[방 생성] /create [방이름] [최대인원]\r\n\
[방 목록] /roomlist\r\n\
[방 내 유저 목록] /userlist\r\n\
[서버 내 유저 목록] /alluserlist\r\n" };
	user->SendChat(helpCmd);
}

void ChatServer::ProcessError(const UserPtr & user)
{
	if (nullptr == user)
	{
		return;
	}
	//명령어 목록에 없는 명령어가 왔을 시 처리.
	static std::string wrongCmd{ "잘못된 명령어 형식입니다." };
	user->SendChat(wrongCmd);
}