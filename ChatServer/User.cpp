#include "User.h"
#include "Room.h"

void User::SendChat(const std::string &msg)
{
	if (m_socket == INVALID_SOCKET)
	{
		std::cout << m_name << "has INVALID_SOCKET" << std::endl;
		return;
	}
	std::string completeMsg = msg + "\r\n";
	send(m_socket, completeMsg.c_str(), static_cast<int>(completeMsg.size()), 0);
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

bool User::GetIsLogin()
{
	return m_login;
}

void User::SetLogin(const std::string& name)
{
	m_login = true;
	m_name = name;
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

UserManager::UserManager() : m_userTable()
{

};