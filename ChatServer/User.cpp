#include "User.h"
#include "Room.h"
#include "Logger.h"

User::User(SOCKET socket, SOCKADDR_IN addr) : m_socket(socket), m_name(), m_data(), m_room(nullptr), m_login(false), m_isAlive(true), m_addr()
{
	char addrArray[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &addr.sin_addr, addrArray, INET_ADDRSTRLEN);
	m_addr = "[" + std::string(addrArray) + ":" + std::to_string(ntohs(addr.sin_port)) + "]";
}

void User::SendChat(const std::string &msg)
{
	//만료된 소켓에는 전송하지 않는다.
	if (INVALID_SOCKET == m_socket)
	{
		Logger::Log("[ERROR] " + m_addr + " - INVALID_SOCKET에 전송");
		return;
	}
	std::string completeMsg = msg + "\r\n";
	int sendSize = send(m_socket, completeMsg.c_str(), static_cast<int>(completeMsg.size()), 0);

	//전송 후 예외처리
	if (sendSize <= 0) // 일반적인 접속종료
	{
		g_userManager.DisconnectUser(m_name);
		if (sendSize < 0) // 에러코드 핸들링
		{
			Logger::WsaLog(m_addr.c_str(), WSAGetLastError());
		}
	}
}

RoomPtr User::GetRoom() const
{
	//유저의 방을 반환.
	return m_room;
}

void User::SetRoom(const RoomPtr &roomPtr)
{
	//유저의 방을 설정.
	m_room = roomPtr;
}

void User::SetName(const std::string & name)
{
	//유저의 이름을 설정.
	m_name = name;
}

void User::SetSocket(SOCKET socket)
{
	//유저의 소켓을 설정.
	m_socket = socket;
}

bool User::IsAlive()
{
	//세션이 살아있는 지 반환.
	return m_isAlive;
}

void User::Kill()
{
	//세션이 살아있다면 소켓 종료처리를 한다.
	if (true == IsAlive())
	{
		SOCKET oldSocket = m_socket;
		m_socket = INVALID_SOCKET;
		m_isAlive = false;
		closesocket(oldSocket);
	}
}

bool User::GetIsLogin()
{
	//로그인 여부를 반환.
	return m_login;
}

void User::SetLogin(const std::string& name)
{
	//이름설정 후 로그인 상태로 전환.
	m_login = true;
	SetName(name);
}

std::string User::GetAddr()
{
	//주소정보 문자열을 반환.
	return m_addr;
}

void User::PushData(const char * data, int length)
{
	if (nullptr == data)
	{
		return;
	}

	//주어진 버퍼에서 순차적으로 데이터 삽입
	for (int i = 0; i < length; ++i)
	{
		if (data[i] == VK_BACK) //백스페이스는 기존 데이터에서 한 글자를 뺀다.
			m_data.pop_back();
		else
			m_data.push_back(data[i]);
	}
}

std::string User::GetName() const
{
	//이름을 반환.
	return "[" + m_name + "]";
}

SOCKET User::GetSocket()
{
	//소켓을 반환.
	return m_socket;
}

UserManager & UserManager::Instance()
{
	//싱글턴 객체 생성 및 호출.
	static UserManager *userManager = new UserManager();
	return *userManager;
}

UserPtr UserManager::GetUser(const std::string &userName)
{
	//이름이 테이블에 있는 지 검색 후 반환. - 그냥 반환하면 nullptr이 테이블에 추가될 수 있다.
	if (0 < m_userTable.count(userName))
	{
		return m_userTable[userName];
	}
	return nullptr;
}

bool UserManager::AddUser(UserPtr& user, const std::string& userName)
{
	if (nullptr == user)
	{
		return false;
	}

	//테이블에 중복 이름이 없는걸 확인 후, 추가하면서 유저 로그인 처리.
	if (0 == m_userTable.count(userName))
	{
		user->SetLogin(userName);
		m_userTable.emplace(userName, user);
		return true;
	}
	//중복 이름이 있으면 false 반환.
	return false;
}

size_t UserManager::EraseUser(const UserPtr & user)
{
	if (nullptr == user) 
	{
		return 0;
	}
	//이름에 해당하는 유저 삭제 후 카운트 반환.
	return m_userTable.erase(user->GetName());
}

std::string UserManager::GetUserList()
{
	std::string userNameList{ "==서버 내 유저 목록==\r\n" };
	for (const auto& user : m_userTable)
	{
		userNameList += "[" + user.first + "]" + "\r\n"; //테이블을 순회하면서 이름 획득
	}
	return userNameList;
}

void UserManager::DisconnectUser(const std::string & userName)
{
	//이름으로 유저를 종료처리하는 경우
	//포인터 획득 후 종료처리
	UserPtr user = GetUser(userName);
	if (nullptr == user)
	{
		return;
	}
	DisconnectUser(user);
}

void UserManager::DisconnectUser(UserPtr & user)
{
	//포인터로 유저를 종료처리하는 경우
	if (nullptr == user)
	{
		return;
	}
	if (true == user->IsAlive()) 
	{
		//방에서 퇴장 후 세션 종료처리 이행
		RoomPtr curRoom = user->GetRoom();
		if (nullptr != curRoom)
		{
			curRoom->Leave(user);
		}
		user->Kill();
		std::string userName = user->GetName();
		EraseUser(user);
		Logger::Log("[USER LOGOUT] " + userName);
	}
}