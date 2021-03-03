#include "Room.h"
#include "User.h"
#include "Logger.h"

constexpr int LOBBY = 0;
Room::Room(const std::string & name, int idx, int maxUser)
	: m_maxUser(maxUser), m_name(name), m_roomIdx(idx)
{
	//�־��� maxUser�� ���� ���� �̸� Ȯ��.
	maxUser = min(maxUser, MAX_RESERVE_USERSIZE);
	m_userList.reserve(maxUser);
}

bool Room::Enter(SOCKET socket, const std::string& name)
{
	// ���� �����ο� Ȯ�� �� ���� �����ϸ� ���� ó��
	if (m_maxUser > m_userTable.size())
	{
		m_userList.emplace_back(UserInfo{ socket, name });
		m_userTable.emplace(socket, m_userList.size() - 1);
		g_userManager.SendMsg(socket, "[������]" + m_name);
		g_userManager.SendMsg(socket, GetUserList());
		NotifyAll("[��������]" + name);
		return true;
	}
	// �ִ��ο� �ʰ��� ���� �Ұ� �� false ��ȯ.
	return false;
}

size_t Room::Leave(SOCKET socket)
{
	//������ �� �ȿ� ������
	if (0 != m_userTable.count(socket))
	{
		//�����Ѵ�.
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
			NotifyAll("[��������]" + userName); // ������ ������ �˸�
		}
	}
	return m_userList.size();
}

void Room::NotifyAll(const std::string& msg)
{
	//�� �� ��� �������� �޼��� �߼�
	for (auto& userInfo : m_userList)
	{
		g_userManager.SendMsg(userInfo.socket, msg);
	}
}

void Room::SendChat(SOCKET socket, const std::string& msg) const
{
	// [����ID] �޼���
	if (0 != m_userTable.count(socket))
	{
		size_t userPos = m_userTable.at(socket);
		std::string completeMsg(m_userList[userPos].name + " : " + msg);
		for (auto& userInfo : m_userList)
		{
			//if (userInfo.socket != socket) //�۽������״� ���� ����.
			//{
				g_userManager.SendMsg(userInfo.socket, completeMsg);
			//}
		}
	}
}

std::string Room::GetUserList()
{
	// ==���� ��� ==
	// [abc]
	// [zipzip]
	// [hungry]
	// ...
	std::string userNameList{ "[�������]\r\n" };
	//�����̳ʸ� ��ȸ�ϸ鼭 �̸� ����.
	for (const UserInfo& user : m_userList)
	{
		userNameList += user.name + "\r\n";
	}
	return userNameList;
}

int RoomManager::CreateRoom(const std::string& name, int maxUser, bool roomLimit)
{
	//�����ο� ������ �ּ��ִ� �� ������ ����ٸ� ���� �Ұ�.
	if (true == roomLimit) {
		if (MINUSER_NUM > maxUser ||
			MAXUSER_NUM < maxUser)
		{
			return OUT_OF_RANGE;
		}
	}

		maxUser = min(maxUser, MAX_LOBBY_SIZE);
		int roomIdx{ 0 };
		if (false == m_reuseRoomCnt.empty())	// ���ÿ��� ���� ������ �ε��� �ִ��� Ȯ��
		{
			roomIdx = m_reuseRoomCnt.top();
			m_reuseRoomCnt.pop();
		}
		else
		{
			roomIdx = m_genRoomCnt++;			// ������ ���� �߱�
		}

		//�� ��ü ���� �� ���̺� ����.
		m_roomList.emplace_back(name, roomIdx, maxUser);
		m_roomTable.emplace(roomIdx, m_roomList.size() - 1);
		Logger::Log("[CREATE ROOM] [" + name + ", MAX_USER : " + std::to_string(maxUser) + "]");

		//������ �� �ε����� ��ȯ
		return roomIdx;
}

Room* RoomManager::GetRoom(size_t idx)
{
	//�˻��ؼ� ���� ������
	if (0 != m_roomTable.count(idx))
	{
		return &m_roomList[m_roomTable[idx]];
	}
	return nullptr;
}

void RoomManager::DestroyRoom(int idx)
{
	if (LOBBY == idx) // �κ�� ������ �ʿ� X
	{
		return;
	}
	if (1 == m_roomTable.count(idx)) // ���� �� �ε����� ���� ����
	{
		Room& room = m_roomList[m_roomTable[idx]];
		if (0 < room.m_userTable.size()) // ������ ���������� ����� �ȵ�
		{
			return;
		}

		//�� ���� �α� ���.
		Logger::Log("[DESTROY ROOM] [" + room.m_name + ", MAX_USER : " + std::to_string(room.m_maxUser) + "]");

		//���̺� Ž�� �� ���̺� ����, ���� ����Ʈ���� ����.
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
	std::string roomNameList{ "[�� ���]\r\n" };
	//m_roomList�� RoomPtr�� vector, �״�� ��ȸ�ϸ� �ȴ�.
	for (const auto& room : m_roomList)
	{
		//��ȸ�ϸ鼭 �� �̸��� ��ȣ �߰�.
		roomNameList += "[" + std::to_string(room.m_roomIdx) + "] " + room.m_name + " (" + std::to_string(room.m_userTable.size()) + "/" + std::to_string(room.m_maxUser) + ")\r\n";
	}
	return roomNameList;
}

RoomManager & RoomManager::Instance()
{
	//�̱��� �ν��Ͻ� ���� �� ��ȯ.
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
