#include "User.h"
#include "Room.h"

void User::SendChat(const std::string &msg)
{
	if (m_socket == INVALID_SOCKET)
	{
		return;
	}
	std::string completeMsg = msg + "\r\n";
	send(m_socket, completeMsg.c_str(), static_cast<int>(completeMsg.size()), 0);
}

RoomPtr User::GetRoom()
{
	return m_room;
}
