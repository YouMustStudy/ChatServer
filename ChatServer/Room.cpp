#include "Room.h"
#include "User.h"

constexpr int LOBBY = 0;

RoomManager* Room::m_roomMgr = nullptr;

void Room::SetWeakPtr(RoomPtr &myself)
{
	if(myself.get() == this)
		m_selfPtr = myself;
}

bool Room::Enter(UserPtr user)
{
	/// ���� �����ο� Ȯ�� �� ����
	if (m_maxUser > m_userTable.size())
	{
		m_userTable.emplace(user);
		user->m_room = m_selfPtr.lock();
		NotifyAll("Welcome " + user->m_name + "!!");
		return true;
	}
	return false;
}

bool Room::Leave(const UserPtr user)
{
	if (1 == m_userTable.erase(user))
	{
		if(user->m_room = m_selfPtr.lock())
			user->m_room = nullptr;

		if (true == m_userTable.empty()) /// �ο����� 0�̸� �� ����
		{
			m_roomMgr->DestroyRoom(m_roomIdx);
		}
		else
		{
			NotifyAll("ByeBye " + user->m_name); /// ������ ������ �˸�
		}
		return true;
	}

	return false;
}

void Room::NotifyAll(const std::string& msg)
{
	/// [Room Name] - �޼���
	std::string completeMsg(std::string("[Room ") + m_name + std::string("] - ") + msg + "\r\n");
	for (auto& userPtr : m_userTable)
	{
		send(userPtr->m_socket, completeMsg.c_str(), static_cast<int>(completeMsg.size()), 0);
	}
}

void Room::SendChat(const UserPtr sender, const std::string& msg)
{
	/// [����ID] : �޼���
	std::string completeMsg(std::string("[") + sender->m_name + std::string("] : ") + msg);
	for (auto& userPtr: m_userTable)
	{
		if (userPtr != sender)
			userPtr->SendChat(msg);
	}
}

std::string Room::GetUserList()
{
	/// ==���� ��� ==
	/// abc
	/// zipzip
	/// hungry
	/// ...
	std::string UserNameList{"==���� ���==\r\n"};
	for (const auto& user : m_userTable)
	{
		UserNameList += user->m_name + "\r\n";
	}
	return UserNameList;
}

RoomManager::RoomManager() : m_genRoomCnt(), m_roomTable()
{
	Initialize();
}

void RoomManager::Initialize()
{
	/// �� ���� ȣ��� ������ ���
	Room::m_roomMgr = this;
}

RoomPtr RoomManager::CreateRoom(const std::string & name)
{
	int roomIdx{0};
	if (false == m_reuseRoomCnt.empty())	/// ���ÿ��� ���� ������ �ε��� �ִ��� Ȯ��
	{
		roomIdx = m_reuseRoomCnt.top();
		m_reuseRoomCnt.pop();
	}
	else
	{
		roomIdx = m_genRoomCnt++;			/// ������ ���� �߱�
	}

	m_roomTable.emplace(std::make_pair(roomIdx, new Room(name, roomIdx)));
	if (nullptr != m_roomTable[roomIdx])
	{
		std::cout << "Room " << name << " is Created" << std::endl;
	}
	return m_roomTable[roomIdx];
}

bool RoomManager::DestroyRoom(int idx)
{
	if (LOBBY == idx) /// �κ�� ������ �ʿ� X
	{
		return false;
	}

	if (0 == m_roomTable.count(idx)) /// ���� �� �ε����� ���� ����
	{
		return false;
	}

	if (m_roomTable[idx]->m_userTable.size() > 0) /// ������ ���������� ����� �ȵ�
	{
		return false;
	}

	std::string roomName = m_roomTable[idx]->m_name;
	size_t success = m_roomTable.erase(idx);
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
	if (0 == m_roomTable.count(idx))
	{
		return nullptr;
	}
	return m_roomTable[idx];
}

std::string RoomManager::GetRoomList()
{
	std::string roomNameList;
	for (const auto& roomPair : m_roomTable)
	{
		roomNameList += "[" + std::to_string(roomPair.first) + "]" + roomPair.second->m_name + "\r\n";
	}
	return roomNameList;
}