#include "Room.h"
#include "User.h"
#include "Logger.h"

constexpr int LOBBY = 0;

void Room::SetWeakPtr(RoomPtr &myself)
{
	//���� Enter, Leave �� �ش� ������ �� �����͸� �����ϱ� ���� �ڽ��� shared_ptr ����.
	//��ȣ������ ���� ���� weak_ptr ���·� ������.
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
	// ���� �����ο� Ȯ�� �� ���� �����ϸ� ���� ó��
	if (m_maxUser > m_userTable.size())
	{
		m_userTable.emplace(user);
		user->SetRoom(m_selfPtr.lock());
		NotifyAll("Welcome " + user->GetName() + "!!");
		return true;
	}
	// �ִ��ο� �ʰ��� ���� �Ұ� �� false ��ȯ.
	return false;
}

bool Room::Leave(UserPtr &user)
{
	if (nullptr == user)
	{
		return false;
	}

	if (1 == m_userTable.erase(user)) //������ �����ϸ�
	{
		RoomPtr userRoom = user->GetRoom();
		if (userRoom == m_selfPtr.lock()) //������ �������͸� nullptr�� ����.
		{
			user->SetRoom(nullptr);
		}
		if (true == m_userTable.empty()) // �ο����� 0�̸� �� ����
		{
			g_roomManager.DestroyRoom(m_roomIdx);
		}
		else
		{
			NotifyAll("ByeBye " + user->GetName()); // ������ ������ �˸�
		}
		return true;
	}

	//���� ���� �� false ��ȯ.
	return false;
}

void Room::NotifyAll(const std::string& msg)
{
	//[ROOM NOTIFY] - �޼���
	std::string completeMsg(std::string("[ROOM NOTIFY] - ") + msg);
	//userPtr�� set�� ����ؼ� 'const'�� �´�. ��ȸ �� ������ ��.
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

	// [����ID] �޼���
	std::string completeMsg(sender->GetName() + " " + msg);
	//userPtr�� set�� ����ؼ� 'const'�� �´�. ��ȸ �� ������ ��.
	for (auto& userPtr: m_userTable)
	{
		if (userPtr != sender) //�۽������״� ���� ����.
			userPtr->SendChat(completeMsg);
	}
}

std::string Room::GetUserList()
{
	// ==���� ��� ==
	// [abc]
	// [zipzip]
	// [hungry]
	// ...
	std::string userNameList{"==�� ������ ���==\r\n"};
	//userPtr�� set�� ����ؼ� 'const'�� �´�. ��ȸ �� ������ ��.
	for (const auto& userPtr : m_userTable)
	{
		userNameList += userPtr->GetName() + "\r\n";
	}
	return userNameList;
}

bool Room::IsSameIdx(int idx)
{
	//������ �ε����� �־��� �ε����� ��ġ�ϴ� �� Ȯ��, ��ȯ.
	return m_roomIdx == idx;
}

RoomManager::RoomManager() : m_genRoomCnt(), m_roomTable()
{
}

RoomPtr RoomManager::CreateRoom(const std::string & name, int maxUser)
{
	//�����ο� ������ �ּ� ������ �۴ٸ� ���� �Ұ�.
	if (maxUser < MINUSER_NUM)
	{
		return nullptr;
	}

	int roomIdx{0};
	if (false == m_reuseRoomCnt.empty())	// ���ÿ��� ���� ������ �ε��� �ִ��� Ȯ��
	{
		roomIdx = m_reuseRoomCnt.top();
		m_reuseRoomCnt.pop();
	}
	else
	{
		roomIdx = m_genRoomCnt++;			// ������ ���� �߱�
	}
	m_roomTable.emplace(std::make_pair(roomIdx, new Room(name, roomIdx, maxUser)));
	if (nullptr != m_roomTable[roomIdx])
	{
		//�� ���� �� �ڱ� �ڽ� ���� ����
		m_roomTable[roomIdx]->SetWeakPtr(m_roomTable[roomIdx]);
		//�α� ���
		Logger::Log("[CREATE ROOM] [" + name + ", MAX_USER : " + std::to_string(maxUser) + "]");
	}
	//������ �� ������ ��ȯ.
	return m_roomTable[roomIdx];
}

bool RoomManager::DestroyRoom(int idx)
{
	if (LOBBY == idx) // �κ�� ������ �ʿ� X
	{
		return false;
	}
	if (0 == m_roomTable.count(idx)) // ���� �� �ε����� ���� ����
	{
		return false;
	}
	if (m_roomTable[idx]->m_userTable.size() > 0) // ������ ���������� ����� �ȵ�
	{
		return false;
	}

	std::string roomName = m_roomTable[idx]->m_name;
	int maxUser = m_roomTable[idx]->m_maxUser;
	size_t success = m_roomTable.erase(idx);
	if (1 == success)
	{
		//�� ���� ���� �� �α� ���.
		Logger::Log("[DESTROY ROOM] [" + roomName + ", MAX_USER : " + std::to_string(maxUser) + "]");
		m_reuseRoomCnt.push(idx);
		return true;
	}
	return false;
}

RoomPtr RoomManager::GetRoom(int idx)
{
	//�˻� �� ������ nullptr ��ȯ.
	if (0 == m_roomTable.count(idx))
	{
		return nullptr;
	}
	return m_roomTable[idx];
}

std::string RoomManager::GetRoomList()
{
	std::string roomNameList{ "==�� ���==\r\n" };
	//roomPair�� map�� ����ؼ� first�� const�� �����ȴ�. ��ȸ �� ������ ��.
	for (const auto& roomPair : m_roomTable)
	{
		//��ȸ�ϸ鼭 �� �̸��� ��ȣ �߰�.
		roomNameList += "[" + std::to_string(roomPair.first) + "] " + roomPair.second->m_name + " (" + std::to_string(roomPair.second->m_userTable.size()) + "/" + std::to_string(roomPair.second->m_maxUser) + ")\r\n";
	}
	return roomNameList;
}

RoomManager & RoomManager::Instance()
{
	//�̱��� �ν��Ͻ� ���� �� ��ȯ.
	static RoomManager *roomManager = new RoomManager();
	return *roomManager;
}
