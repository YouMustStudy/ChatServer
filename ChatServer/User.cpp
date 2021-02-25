#include "User.h"
#include "Room.h"
#include "Error.h"

User::User(SOCKET socket, SOCKADDR_IN addr) : m_socket(socket), m_name(), m_data(), m_room(nullptr), m_login(false), m_isAlive(true), m_addr()
{
	char addrArray[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &addr, addrArray, INET_ADDRSTRLEN);
	m_name = "[" + std::string(addrArray) + ":" + std::to_string(ntohs(addr.sin_port)) + "]";
}

void User::SendChat(const std::string &msg)
{
	//����� ���Ͽ��� �������� �ʴ´�.
	if (INVALID_SOCKET == m_socket)
	{
		std::cout << m_name << "has INVALID_SOCKET" << std::endl;
		return;
	}
	std::string completeMsg = msg + "\r\n";
	int sendSize = send(m_socket, completeMsg.c_str(), static_cast<int>(completeMsg.size()), 0);

	//���� �� ����ó��
	if (sendSize <= 0) // �Ϲ����� ��������
	{
		g_userManager.DisconnectUser(m_name);
		if (sendSize < 0)
		{
			int errCode = WSAGetLastError();
			error_display(m_addr.c_str(), errCode);
		}
	}
}

RoomPtr User::GetRoom() const
{
	//������ ���� ��ȯ.
	return m_room;
}

void User::SetRoom(const RoomPtr &roomPtr)
{
	//������ ���� ����.
	m_room = roomPtr;
}

void User::SetName(const std::string & name)
{
	//������ �̸��� ����.
	m_name = name;
}

void User::SetSocket(SOCKET socket)
{
	//������ ������ ����.
	m_socket = socket;
}

bool User::IsAlive()
{
	//������ ����ִ� �� ��ȯ.
	return m_isAlive;
}

void User::Kill()
{
	//������ ����ִٸ� ���� ����ó���� �Ѵ�.
	if (true == IsAlive())
	{
		SOCKET oldSocket = m_socket;
		m_socket = INVALID_SOCKET;
		m_isAlive = false;
		closesocket(oldSocket);
	}
}

bool User::GetIsLogin()
{
	//�α��� ���θ� ��ȯ.
	return m_login;
}

void User::SetLogin(const std::string& name)
{
	//�̸����� �� �α��� ���·� ��ȯ.
	m_login = true;
	m_name = name;
}

std::string User::GetAddr()
{
	//�ּ����� ���ڿ��� ��ȯ.
	return m_addr;
}

void User::PushData(const char * data, int length)
{
	if (nullptr == data)
	{
		return;
	}

	//�־��� ���ۿ��� ���������� ������ ����
	for (int i = 0; i < length; ++i)
	{
		if (data[i] == VK_BACK) //�齺���̽��� ���� �����Ϳ��� �� ���ڸ� ����.
			m_data.pop_back();
		else
			m_data.push_back(data[i]);
	}
}

std::string User::GetName() const
{
	//�̸��� ��ȯ.
	return m_name;
}

SOCKET User::GetSocket()
{
	//������ ��ȯ.
	return m_socket;
}

UserManager & UserManager::Instance()
{
	//�̱��� ��ü ���� �� ȣ��.
	static UserManager *userManager = new UserManager();
	return *userManager;
}

UserPtr UserManager::GetUser(const std::string &userName)
{
	//�̸��� ���̺� �ִ� �� �˻� �� ��ȯ. - �׳� ��ȯ�ϸ� nullptr�� ���̺� �߰��� �� �ִ�.
	if (0 < m_userTable.count(userName))
	{
		return m_userTable[userName];
	}
	return nullptr;
}

bool UserManager::AddUser(UserPtr& user, const std::string& userName)
{
	if (nullptr == user)
	{
		return false;
	}

	//���̺� �ߺ� �̸��� ���°� Ȯ�� ��, �߰��ϸ鼭 ���� �α��� ó��.
	if (0 == m_userTable.count(userName))
	{
		user->SetLogin(userName);
		m_userTable.emplace(userName, user);
		return true;
	}
	//�ߺ� �̸��� ������ false ��ȯ.
	return false;
}

size_t UserManager::EraseUser(const UserPtr & user)
{
	if (nullptr == user) 
	{
		return 0;
	}
	//�̸��� �ش��ϴ� ���� ���� �� ī��Ʈ ��ȯ.
	return m_userTable.erase(user->GetName());
}

std::string UserManager::GetUserList()
{
	std::string userNameList{ "==���� �� ���� ���==\r\n" };
	for (const auto& user : m_userTable)
	{
		userNameList += "[" + user.first + "]" + "\r\n"; //���̺��� ��ȸ�ϸ鼭 �̸� ȹ��
	}
	return userNameList;
}

void UserManager::DisconnectUser(const std::string & userName)
{
	//�̸����� ������ ����ó���ϴ� ���
	//������ ȹ�� �� ����ó��
	UserPtr user = GetUser(userName);
	if (nullptr == user)
	{
		return;
	}
	DisconnectUser(user);
}

void UserManager::DisconnectUser(UserPtr & user)
{
	//�����ͷ� ������ ����ó���ϴ� ���
	if (nullptr == user)
	{
		return;
	}
	if (true == user->IsAlive()) 
	{
		//�濡�� ���� �� ���� ����ó�� ����
		RoomPtr curRoom = user->GetRoom();
		curRoom->Leave(user);
		user->Kill();
		g_userManager.EraseUser(user);
		std::cout << "[USER LOGOUT] - " << user->GetName() << std::endl;
	}
}