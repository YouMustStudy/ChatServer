#include "User.h"
#include "Room.h"
#include "Error.h"

User::User(SOCKET socket, SOCKADDR_IN addr) : m_socket(socket), m_name(), m_data(), m_room(nullptr), m_login(false), m_isAlive(true), m_addr()
{
	char addrArray[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &addr, addrArray, INET_ADDRSTRLEN);
	m_name = "[" + std::string(addrArray) + ":" + std::to_string(ntohs(addr.sin_port)) + "]";
}

void User::SendChat(const std::string &msg)
{

	if (INVALID_SOCKET == m_socket)
	{
		std::cout << m_name << "has INVALID_SOCKET" << std::endl;
		return;
	}
	std::string completeMsg = msg + "\r\n";
	int sendSize = send(m_socket, completeMsg.c_str(), static_cast<int>(completeMsg.size()), 0);
	if (sendSize == 0)
	{
		g_userManager.DisconnectUser(m_name);
	}
	else if (sendSize < 0) 
	{
		int errCode = WSAGetLastError();
		if (EAGAIN != errCode)
		{
			error_display(m_addr.c_str() ,errCode);
		}
	}
}

RoomPtr User::GetRoom()
{
	return m_room;
}

void User::SetRoom(const RoomPtr &roomPtr)
{
	m_room = roomPtr;
}

void User::SetName(const std::string & name)
{
	m_name = name;
}

void User::SetSocket(SOCKET socket)
{
	m_socket = socket;
}

bool User::IsAlive()
{
	return m_isAlive;
}

void User::Kill()
{
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
	return m_login;
}

void User::SetLogin(const std::string& name)
{
	m_login = true;
	m_name = name;
}

std::string User::GetAddr()
{
	return m_addr;
}

void User::PushData(const char * data, int length)
{
	for (int i = 0; i < length; ++i)
	{
		if (data[i] == VK_BACK)
			m_data.pop_back();
		else
			m_data.push_back(data[i]);
	}
}

std::string User::GetName()
{
	return m_name;
}

SOCKET User::GetSocket()
{
	return m_socket;
}

UserManager & UserManager::Instance()
{
	static UserManager *userManager = new UserManager();
	return *userManager;
}

UserPtr UserManager::GetUser(const std::string &userName)
{
	if (0 < m_userTable.count(userName))
	{
		return m_userTable[userName];
	}
	return nullptr;
}

bool UserManager::AddUser(UserPtr& user, const std::string& userName)
{
	if (0 == m_userTable.count(userName))
	{
		user->SetLogin(userName);
		m_userTable.emplace(userName, user);
		return true;
	}
	return false;
}

size_t UserManager::EraseUser(const UserPtr & user)
{
	return m_userTable.erase(user->GetName());
}

std::string UserManager::GetUserList()
{
	std::string userNameList{ "==서버 내 유저 목록==\r\n" };
	for (const auto& user : m_userTable)
	{
		userNameList += "[" + user.first + "]" + "\r\n";
	}
	return userNameList;
}

void UserManager::DisconnectUser(const std::string & userName)
{
	UserPtr user = g_userManager.GetUser(userName);
	if (nullptr == user)
	{
		return;
	}
	DisconnectUser(user);
}

void UserManager::DisconnectUser(UserPtr & user)
{
	if (nullptr == user)
	{
		return;
	}
	if (true == user->IsAlive()) 
	{
		RoomPtr curRoom = user->GetRoom();
		curRoom->Leave(user);
		user->Kill();
		g_userManager.EraseUser(user);
		std::cout << "[USER LOGOUT] - " << user->GetName() << std::endl;
	}
}

UserManager::UserManager() : m_userTable()
{

};