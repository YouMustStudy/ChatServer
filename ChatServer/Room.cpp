#include "Room.h"
#include "User.h"

constexpr int LOBBY = 0;

RoomManager* Room::m_roomMgr = nullptr;

void Room::SetWeakPtr(RoomPtr &myself)
{
	if (myself.get() == this) 
	{
		m_selfPtr = myself;
	}
}

bool Room::Enter(UserPtr &user)
{
	/// 방의 입장인원 확인 후 입장
	if (m_maxUser > m_userTable.size())
	{
		m_userTable.emplace(user);
		user->SetRoom(m_selfPtr.lock());
		NotifyAll("Welcome " + user->GetName() + "!!");
		return true;
	}
	return false;
}

bool Room::Leave(UserPtr &user)
{
	if (1 == m_userTable.erase(user))
	{
		RoomPtr userRoom = user->GetRoom();
		if (userRoom == m_selfPtr.lock())
		{
			user->SetRoom(nullptr);
		}
		if (true == m_userTable.empty()) /// 인원수가 0이면 방 삭제
		{
			m_roomMgr->DestroyRoom(m_roomIdx);
		}
		else
		{
			NotifyAll("ByeBye " + user->GetName()); /// 유저의 퇴장을 알림
		}
		return true;
	}

	return false;
}

void Room::NotifyAll(const std::string& msg)
{
	/// [Room Name] - 메세지
	std::string completeMsg(std::string("[Room ") + m_name + std::string("] - ") + msg);
	for (auto& userPtr : m_userTable)
	{
		userPtr->SendChat(completeMsg);
	}
}

void Room::SendChat(const UserPtr sender, const std::string& msg)
{
	/// [유저ID] : 메세지
	std::string completeMsg(std::string("[") + sender->GetName() + std::string("] : ") + msg);
	for (auto& userPtr: m_userTable)
	{
		if (userPtr != sender)
			userPtr->SendChat(completeMsg);
	}
}

std::string Room::GetUserList()
{
	/// ==유저 목록 ==
	/// abc
	/// zipzip
	/// hungry
	/// ...
	std::string userNameList{"==유저 목록==\r\n"};
	for (const auto& user : m_userTable)
	{
		userNameList += "[" + user->GetName() + "]" + "\r\n";
	}
	return userNameList;
}

RoomManager::RoomManager() : m_genRoomCnt(), m_roomTable()
{
	Initialize();
}

void RoomManager::Initialize()
{
	/// 방 삭제 호출용 포인터 등록
	Room::m_roomMgr = this;
}

RoomPtr RoomManager::CreateRoom(const std::string & name, int maxUser)
{
	int roomIdx{0};
	if (false == m_reuseRoomCnt.empty())	/// 스택에서 재사용 가능한 인덱스 있는지 확인
	{
		roomIdx = m_reuseRoomCnt.top();
		m_reuseRoomCnt.pop();
	}
	else
	{
		roomIdx = m_genRoomCnt++;			/// 없으면 새로 발급
	}

	m_roomTable.emplace(std::make_pair(roomIdx, new Room(name, roomIdx, maxUser)));
	if (nullptr != m_roomTable[roomIdx])
	{
		m_roomTable[roomIdx]->SetWeakPtr(m_roomTable[roomIdx]);
		std::cout << "Room " << name << " is created" << std::endl;
	}
	return m_roomTable[roomIdx];
}

bool RoomManager::DestroyRoom(int idx)
{
	if (LOBBY == idx) /// 로비면 삭제할 필요 X
	{
		return false;
	}

	if (0 == m_roomTable.count(idx)) /// 없는 방 인덱스로 삭제 방지
	{
		return false;
	}

	if (m_roomTable[idx]->m_userTable.size() > 0) /// 유저가 남아있으면 지우면 안됨
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
	std::string roomNameList{ "==방 목록==\r\n" };
	for (const auto& roomPair : m_roomTable)
	{
		roomNameList += "[" + std::to_string(roomPair.first) + "] " + roomPair.second->m_name + "\r\n";
	}
	return roomNameList;
}