#include "Room.h"
#include "User.h"

bool Room::Enter(const UserPtr user)
{
	if (maxUser > m_userList.size())
	{
		m_userList.emplace(user);
		NotifyAll("Welcome " + user->id + "!!");
		return true;
	}
	return false;
}

bool Room::Leave(const UserPtr user)
{
	if (1 == m_userList.erase(user))
	{
		NotifyAll("ByeBye " + user->id);
		return true;
	}
	return false;
}


void Room::NotifyAll(const std::string& msg)
{
	std::string completeMsg(std::string("[Room ") + id + std::string("] : ") + msg + "\r\n");
	for (auto& userPtr : m_userList)
	{
		send(userPtr->m_socket, completeMsg.c_str(), static_cast<int>(completeMsg.size()), 0);
	}
}

void Room::SendChat(const UserPtr sender, const std::string& msg)
{
	std::string completeMsg(std::string("[") + sender->id + std::string("] : ") + msg);
	for (auto& userPtr: m_userList)
	{
		if(userPtr != sender)
			send(userPtr->m_socket, completeMsg.c_str(), static_cast<int>(completeMsg.size()), 0);
	}
}