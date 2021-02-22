#include "Room.h"
#include "User.h"

constexpr int LOBBY = 0;

RoomManager* Room::m_roomMgr = nullptr;
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

		if (true == m_userList.empty())
		{
			m_roomMgr->DestroyRoom(m_roomIdx);
		}

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

RoomManager::RoomManager() : m_genRoomCnt(), m_roomList()
{
	Initialize();
}

void RoomManager::Initialize()
{
	Room::m_roomMgr = this;
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

	m_roomList.emplace(std::make_pair(roomIdx, new Room(name, roomIdx)));
	std::cout << "Room " << name << " is Created" << std::endl;
	return m_roomList[roomIdx];
}

bool RoomManager::DestroyRoom(int idx)
{
	if (LOBBY == idx) /// 로비면 삭제할 필요 X
	{
		return false;
	}

	if (0 == m_roomList.count(idx)) /// 없는 방 인덱스로 삭제 시도
	{
		return false;
	}

	if (m_roomList[idx]->m_userList.size() > 0) /// 유저가 남아있으면 지우면 안됨
	{
		return false;
	}

	std::string roomName = m_roomList[idx]->m_name;
	size_t success = m_roomList.erase(idx);
	if (1 == success)
	{
		std::cout << "Room " << roomName << " is destroyed" << std::endl;
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

	return m_roomList[idx];
}

std::string RoomManager::GetRoomList()
{
	std::string roomNameList;
	for (const auto& roomPair : m_roomList)
	{
		roomNameList += "[" + std::to_string(roomPair.first) + "]" + roomPair.second->m_name + "\r\n";
	}
	return roomNameList;
}