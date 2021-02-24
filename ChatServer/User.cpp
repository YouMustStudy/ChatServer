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

void User::SetName(std::string & name)
{
	m_name = name;
}

void User::SetSocket(SOCKET socket)
{
	m_socket = socket;
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
