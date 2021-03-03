#include "Room.h"
#include "User.h"
#include "Logger.h"

constexpr int LOBBY = 0;
Room::Room(const std::string & name, int idx, int maxUser)
	: m_maxUser(maxUser), m_name(name), m_roomIdx(idx)
{
	//주어진 maxUser로 벡터 공간 미리 확보.
	maxUser = min(maxUser, MAX_RESERVE_USERSIZE);
	m_userList.reserve(maxUser);
}

bool Room::Enter(SOCKET socket, const std::string& name)
{
	// 방의 입장인원 확인 후 입장 가능하면 이후 처리
	if (m_maxUser > m_userTable.size())
	{
		m_userList.emplace_back(UserInfo{ socket, name });
		m_userTable.emplace(socket, m_userList.size() - 1);
		g_userManager.SendMsg(socket, "[방입장]" + m_name);
		g_userManager.SendMsg(socket, GetUserList());
		NotifyAll("[유저입장]" + name);
		return true;
	}
	// 최대인원 초과로 입장 불가 시 false 반환.
	return false;
}

size_t Room::Leave(SOCKET socket)
{
	//유저가 방 안에 있으면
	if (0 != m_userTable.count(socket))
	{
		//삭제한다.
		size_t userPos = m_userTable[socket];
		std::string userName = m_userList[userPos].name;
		m_userTable.erase(socket);

		if (userPos != m_userList.size() - 1) {
			UserInfo& mover = m_userList.back();
			m_userList[userPos] = mover;
			m_userTable[mover.socket] = userPos;
		}
		m_userList.pop_back();

		if (false == m_userList.empty())
		{
			NotifyAll("[유저퇴장]" + userName); // 유저의 퇴장을 알림
		}
	}
	return m_userList.size();
}

void Room::NotifyAll(const std::string& msg)
{
	//방 내 모든 유저에게 메세지 발송
	for (auto& userInfo : m_userList)
	{
		g_userManager.SendMsg(userInfo.socket, msg);
	}
}

void Room::SendChat(SOCKET socket, const std::string& msg) const
{
	// [유저ID] 메세지
	if (0 != m_userTable.count(socket))
	{
		size_t userPos = m_userTable.at(socket);
		std::string completeMsg(m_userList[userPos].name + " : " + msg);
		for (auto& userInfo : m_userList)
		{
			//if (userInfo.socket != socket) //송신자한테는 전송 안함.
			//{
				g_userManager.SendMsg(userInfo.socket, completeMsg);
			//}
		}
	}
}

std::string Room::GetUserList()
{
	// ==유저 목록 ==
	// [abc]
	// [zipzip]
	// [hungry]
	// ...
	std::string userNameList{ "[유저목록]\r\n" };
	//컨테이너를 순회하면서 이름 수집.
	for (const UserInfo& user : m_userList)
	{
		userNameList += user.name + "\r\n";
	}
	return userNameList;
}

int RoomManager::CreateRoom(const std::string& name, int maxUser, bool roomLimit)
{
	//입장인원 제한이 최소최대 수 제한을 벗어난다면 생성 불가.
	if (true == roomLimit) {
		if (MINUSER_NUM > maxUser ||
			MAXUSER_NUM < maxUser)
		{
			return OUT_OF_RANGE;
		}
	}

		maxUser = min(maxUser, MAX_LOBBY_SIZE);
		int roomIdx{ 0 };
		if (false == m_reuseRoomCnt.empty())	// 스택에서 재사용 가능한 인덱스 있는지 확인
		{
			roomIdx = m_reuseRoomCnt.top();
			m_reuseRoomCnt.pop();
		}
		else
		{
			roomIdx = m_genRoomCnt++;			// 없으면 새로 발급
		}

		//방 객체 생성 후 테이블 매핑.
		m_roomList.emplace_back(name, roomIdx, maxUser);
		m_roomTable.emplace(roomIdx, m_roomList.size() - 1);
		Logger::Log("[CREATE ROOM] [" + name + ", MAX_USER : " + std::to_string(maxUser) + "]");

		//생성된 방 인덱스를 반환
		return roomIdx;
}

Room* RoomManager::GetRoom(size_t idx)
{
	//검색해서 방이 있으면
	if (0 != m_roomTable.count(idx))
	{
		return &m_roomList[m_roomTable[idx]];
	}
	return nullptr;
}

void RoomManager::DestroyRoom(int idx)
{
	if (LOBBY == idx) // 로비면 삭제할 필요 X
	{
		return;
	}
	if (1 == m_roomTable.count(idx)) // 없는 방 인덱스로 삭제 방지
	{
		Room& room = m_roomList[m_roomTable[idx]];
		if (0 < room.m_userTable.size()) // 유저가 남아있으면 지우면 안됨
		{
			return;
		}

		//방 삭제 로그 출력.
		Logger::Log("[DESTROY ROOM] [" + room.m_name + ", MAX_USER : " + std::to_string(room.m_maxUser) + "]");

		//테이블 탐색 후 테이블 삭제, 이후 리스트에서 삭제.
		size_t roomPos = m_roomTable[idx];
		m_roomTable.erase(idx);
		if (roomPos != m_roomList.size() - 1)
		{
			Room& mover = m_roomList.back();
			m_roomList[roomPos] = mover;
			m_roomTable[mover.m_roomIdx] = roomPos;
		}
		m_roomList.pop_back();
		m_reuseRoomCnt.push(idx);
	}
}

std::string RoomManager::GetRoomList()
{
	std::string roomNameList{ "[방 목록]\r\n" };
	//m_roomList는 RoomPtr의 vector, 그대로 순회하면 된다.
	for (const auto& room : m_roomList)
	{
		//순회하면서 방 이름과 번호 추가.
		roomNameList += "[" + std::to_string(room.m_roomIdx) + "] " + room.m_name + " (" + std::to_string(room.m_userTable.size()) + "/" + std::to_string(room.m_maxUser) + ")\r\n";
	}
	return roomNameList;
}

RoomManager & RoomManager::Instance()
{
	//싱글턴 인스턴스 생성 및 반환.
	static RoomManager *roomManager = new RoomManager();
	return *roomManager;
}

bool RoomManager::Enter(SOCKET socket, const std::string& name, int idx)
{
	if (0 != m_roomTable.count(idx))
	{
		size_t roomPos = m_roomTable[idx];
		return m_roomList[roomPos].Enter(socket, name);
	}
	return false;
}

bool RoomManager::Leave(SOCKET socket, size_t idx)
{
	if (0 != m_roomTable.count(idx))
	{
		size_t roomPos = m_roomTable[idx];
		if (0 == m_roomList[roomPos].Leave(socket)) {
			DestroyRoom(idx);
		}
		return true;
	}
	return false;
}
