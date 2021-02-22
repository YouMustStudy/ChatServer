#include "Room.h"
#include "User.h"

bool Room::Enter(const UserPtr user)
{
	if (m_maxUser > m_userList.size())
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
	std::string completeMsg(std::string("[Room ") + m_name + std::string("] : ") + msg + "\r\n");
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

std::string Room::GetUserList()
{
	std::string UserNameList{"==유저 목록==\r\n"};
	for (const auto& user : m_userList)
	{
		UserNameList += user->id + "\r\n";
	}
	return UserNameList;
}

RoomPtr RoomManager::CreateRoom(const std::string & name)
{
	int roomIdx{0};
	if (false == m_reuseRoomCnt.empty())
	{
		roomIdx = m_reuseRoomCnt.top();
		m_reuseRoomCnt.pop();
	}
	else
	{
		roomIdx = m_genRoomCnt++;
	}

	m_roomList.emplace(std::make_pair(roomIdx, Room(name, roomIdx)));
	return &m_roomList[roomIdx];
}

bool RoomManager::DestroyRoom(int idx)
{
	if (0 == m_roomList.count(idx))
	{
		return false;
	}

	Room& room = m_roomList[idx];
	if (room.m_userList.size() > 0)
	{
		return false;
	}

	size_t success = m_roomList.erase(idx);
	if (1 == success)
	{
		m_reuseRoomCnt.push(idx);
		return true;
	}
	return false;
}

RoomPtr RoomManager::GetRoom(int idx)
{
	if (0 == m_roomList.count(idx))
	{
		return nullptr;
	}

	return &m_roomList[idx];
}

std::string RoomManager::GetRoomList()
{
	std::string roomNameList;
	for (const auto& roomPair : m_roomList)
	{
		roomNameList += "[" + std::to_string(roomPair.first) + "]" + roomPair.second.m_name + "\r\n";
	}
	return roomNameList;
}