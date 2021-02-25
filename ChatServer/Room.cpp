#include "Room.h"
#include "User.h"
#include "Logger.h"

constexpr int LOBBY = 0;

void Room::SetWeakPtr(RoomPtr &myself)
{
	//유저 Enter, Leave 시 해당 유저의 방 포인터를 수정하기 위해 자신의 shared_ptr 저장.
	//상호참조를 막기 위해 weak_ptr 형태로 저장함.
	if (nullptr == myself)
	{
		return;
	}
	if (myself.get() == this) 
	{
		m_selfPtr = myself;
	}
}

bool Room::Enter(UserPtr &user)
{
	// 방의 입장인원 확인 후 입장 가능하면 이후 처리
	if (m_maxUser > m_userTable.size())
	{
		m_userTable.emplace(user);
		user->SetRoom(m_selfPtr.lock());
		NotifyAll("Welcome " + user->GetName() + "!!");
		return true;
	}
	// 최대인원 초과로 입장 불가 시 false 반환.
	return false;
}

bool Room::Leave(UserPtr &user)
{
	if (nullptr == user)
	{
		return false;
	}

	if (1 == m_userTable.erase(user)) //삭제가 성공하면
	{
		RoomPtr userRoom = user->GetRoom();
		if (userRoom == m_selfPtr.lock()) //유저의 방포인터를 nullptr로 세팅.
		{
			user->SetRoom(nullptr);
		}
		if (true == m_userTable.empty()) // 인원수가 0이면 방 삭제
		{
			g_roomManager.DestroyRoom(m_roomIdx);
		}
		else
		{
			NotifyAll("ByeBye " + user->GetName()); // 유저의 퇴장을 알림
		}
		return true;
	}

	//삭제 실패 시 false 반환.
	return false;
}

void Room::NotifyAll(const std::string& msg)
{
	//[ROOM NOTIFY] - 메세지
	std::string completeMsg(std::string("[ROOM NOTIFY] - ") + msg);
	//userPtr은 set을 사용해서 'const'로 온다. 순회 시 주의할 것.
	for (auto& userPtr : m_userTable)
	{
		userPtr->SendChat(completeMsg);
	}
}

void Room::SendChat(const UserPtr& sender, const std::string& msg)
{
	if (nullptr == sender)
	{
		return;
	}

	// [유저ID] 메세지
	std::string completeMsg(sender->GetName() + " " + msg);
	//userPtr은 set을 사용해서 'const'로 온다. 순회 시 주의할 것.
	for (auto& userPtr: m_userTable)
	{
		if (userPtr != sender) //송신자한테는 전송 안함.
			userPtr->SendChat(completeMsg);
	}
}

std::string Room::GetUserList()
{
	// ==유저 목록 ==
	// [abc]
	// [zipzip]
	// [hungry]
	// ...
	std::string userNameList{"==방 참여자 목록==\r\n"};
	//userPtr은 set을 사용해서 'const'로 온다. 순회 시 주의할 것.
	for (const auto& userPtr : m_userTable)
	{
		userNameList += userPtr->GetName() + "\r\n";
	}
	return userNameList;
}

bool Room::IsSameIdx(int idx)
{
	//본인의 인덱스와 주어진 인덱스가 일치하는 지 확인, 반환.
	return m_roomIdx == idx;
}

RoomManager::RoomManager() : m_genRoomCnt(), m_roomTable()
{
}

RoomPtr RoomManager::CreateRoom(const std::string & name, int maxUser)
{
	//입장인원 제한이 최소 수보다 작다면 생성 불가.
	if (maxUser < MINUSER_NUM)
	{
		return nullptr;
	}

	int roomIdx{0};
	if (false == m_reuseRoomCnt.empty())	// 스택에서 재사용 가능한 인덱스 있는지 확인
	{
		roomIdx = m_reuseRoomCnt.top();
		m_reuseRoomCnt.pop();
	}
	else
	{
		roomIdx = m_genRoomCnt++;			// 없으면 새로 발급
	}
	m_roomTable.emplace(std::make_pair(roomIdx, new Room(name, roomIdx, maxUser)));
	if (nullptr != m_roomTable[roomIdx])
	{
		//방 생성 후 자기 자신 참조 설정
		m_roomTable[roomIdx]->SetWeakPtr(m_roomTable[roomIdx]);
		//로그 출력
		Logger::Log("[CREATE ROOM] [" + name + ", MAX_USER : " + std::to_string(maxUser) + "]");
	}
	//생성된 방 포인터 반환.
	return m_roomTable[roomIdx];
}

bool RoomManager::DestroyRoom(int idx)
{
	if (LOBBY == idx) // 로비면 삭제할 필요 X
	{
		return false;
	}
	if (0 == m_roomTable.count(idx)) // 없는 방 인덱스로 삭제 방지
	{
		return false;
	}
	if (m_roomTable[idx]->m_userTable.size() > 0) // 유저가 남아있으면 지우면 안됨
	{
		return false;
	}

	std::string roomName = m_roomTable[idx]->m_name;
	int maxUser = m_roomTable[idx]->m_maxUser;
	size_t success = m_roomTable.erase(idx);
	if (1 == success)
	{
		//방 삭제 성공 시 로그 출력.
		Logger::Log("[DESTROY ROOM] [" + roomName + ", MAX_USER : " + std::to_string(maxUser) + "]");
		m_reuseRoomCnt.push(idx);
		return true;
	}
	return false;
}

RoomPtr RoomManager::GetRoom(int idx)
{
	//검색 후 없으면 nullptr 반환.
	if (0 == m_roomTable.count(idx))
	{
		return nullptr;
	}
	return m_roomTable[idx];
}

std::string RoomManager::GetRoomList()
{
	std::string roomNameList{ "==방 목록==\r\n" };
	//roomPair은 map을 사용해서 first는 const로 고정된다. 순회 시 주의할 것.
	for (const auto& roomPair : m_roomTable)
	{
		//순회하면서 방 이름과 번호 추가.
		roomNameList += "[" + std::to_string(roomPair.first) + "] " + roomPair.second->m_name + " (" + std::to_string(roomPair.second->m_userTable.size()) + "/" + std::to_string(roomPair.second->m_maxUser) + ")\r\n";
	}
	return roomNameList;
}

RoomManager & RoomManager::Instance()
{
	//싱글턴 인스턴스 생성 및 반환.
	static RoomManager *roomManager = new RoomManager();
	return *roomManager;
}
