#include "User.h"
#include "Room.h"
#include "Logger.h"
#include "ChatServer.h"

User::User(SOCKET soc, std::string addr) : m_socket(soc), m_addr(addr)
{
}

void User::SendChat(const std::string &msg)
{
	static std::string suffix = std::string("\r\n") + char(5);
	//만료된 소켓에는 전송하지 않는다.
	if (INVALID_SOCKET == m_socket)
	{
		Logger::Log("[ERROR] " + m_addr + " - INVALID_SOCKET에 전송");
		return;
	}

	//참조카운트 증가.
	++m_refCnt;
	std::string completeMsg = msg + suffix;
	//SendJob 포스팅.
	g_chatServer.PushThreadJob(new MainJob{ CMD_SEND, m_socket, new std::string((completeMsg))});
}

User* UserManager::GetUser(SOCKET socket)
{
	if (0 != m_SocketTable.count(socket))
	{
		return &m_userList[m_SocketTable[socket]];
	}
	return nullptr;
}

User* UserManager::GetUser(const std::string& name)
{
	if (0 != m_userTable.count(name))
	{
		return &m_userList[m_userTable[name]];
	}
	return nullptr;
}

void UserManager::AddUser(SOCKET socket, const std::string& addr)
{
	if (0 == m_SocketTable.count(socket))
	{
		m_userList.emplace_back(socket, addr); //UserList에 생성
		m_SocketTable.emplace(socket, m_userList.size() - 1); //현재 UserList의 최대값을 소켓 인덱스로 가진다.
	}
	else
	{
		Logger::Log("[Error] 유저 객체 생성 실패");
	}
}

bool UserManager::Login(SOCKET socket, const std::string& name)
{
	if (0 != m_SocketTable.count(socket))
	{
		if (0 == m_userTable.count(name))
		{
			size_t userPos = m_SocketTable[socket];
			User& user = m_userList[userPos];
			user.m_login = true;
			user.m_name = name;
			m_userTable.emplace(user.m_name, userPos);
			return true;
		}
	}
	return false;
}

void UserManager::SendMsg(SOCKET socket, const std::string& msg)
{
	if (0 != m_SocketTable.count(socket))
	{
		m_userList[m_SocketTable[socket]].SendChat(msg);
	}
}

void UserManager::DisconnectUser(SOCKET socket)
{
	if (0 != m_SocketTable.count(socket))
	{
		size_t userPos = m_SocketTable[socket];
		User& user = m_userList[userPos];

		// 방 퇴장처리
		g_roomManager.Leave(user.m_socket, user.m_room);

		closesocket(user.m_socket);
		m_userTable.erase(user.m_name);
		m_SocketTable.erase(user.m_socket);
		Logger::Log("[USER LOGOUT] " + user.m_name);

		//테이블 매핑 수정 및 삭제.
		//맨 마지막 객체가 아니라면
		if (userPos != m_userList.size() - 1)
		{
			User& mover = m_userList.back();
			m_userTable[mover.m_name] = userPos;
			m_SocketTable[mover.m_socket] = userPos;
			m_userList[userPos] = mover;
		}
		m_userList.pop_back();
	}
}

void UserManager::DecreaseUser(SOCKET socket)
{
	if (0 != m_SocketTable.count(socket))
	{
		size_t userPos = m_SocketTable[socket];
		int leftCnt = --m_userList[userPos].m_refCnt;
		if (0 == leftCnt)
		{
			DisconnectUser(socket);
		}
	}
}

UserManager & UserManager::Instance()
{
	//싱글턴 객체 생성 및 호출.
	static UserManager *userManager = new UserManager();
	return *userManager;
}

std::string UserManager::GetUserList()
{
	std::string userNameList{ "==서버 내 유저 목록==\r\n" };
	for (const auto& user : m_userList)
	{
		userNameList += "[" + user.m_name + "]" + "\r\n"; //테이블을 순회하면서 이름 획득
	}
	return userNameList;
}