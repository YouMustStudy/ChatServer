#pragma once
#include <set>
#include <map>
#include <string>
#include <stack>
#include <memory>
#include <iostream>

class User;
typedef std::shared_ptr<User> UserPtr;

class RoomManager;
/*
Room
�������� ���̴� �ϳ��� ����
ä���� �ش� �� �������� �̷�����.
*/

class Room
{
	friend RoomManager;
	typedef std::set<UserPtr> UserTable;
	typedef std::shared_ptr<Room> RoomPtr;

public:
	Room() : m_userTable(), m_maxUser(INT_MAX), m_name(), m_roomIdx(), m_destroyed(false) {};
	Room(const std::string& name, int idx, int maxUser) : m_userTable(), m_maxUser(maxUser), m_name(name), m_roomIdx(idx), m_destroyed(false) {};
	~Room() {};

	/// �ڱ� �ڽ��� weak_ptr�� �����ϴ� �Լ�, ���� : myself - ������ shared_ptr
	void SetWeakPtr(RoomPtr& myself);
	///������ �濡 ����. ���� : user - ������ ������ ������, ��ȯ : ��������
	bool Enter(UserPtr &user);
	///������ �濡�� ����. ���� : user - ������ ������ ������, ��ȯ : ��������
	bool Leave(UserPtr &user);
	///�� ���� ��� �����鿡�� ���� ����. ���� : msg - ���� �޼���
	void NotifyAll(const std::string& msg);
	///�� ���� ��� �����鿡�� ���� �޼��� ����. ���� : sender - ������ ���, msg - ���� �޼���
	void SendChat(const UserPtr sender, const std::string& msg);
	///�� ���� ��� ���� �̸� ��ȯ. ��ȯ : �� ���� ���� �̸���
	std::string GetUserList();
	///�־��� �ε����� ������ �� ��. ���� : idx - ���� �ε���
	bool IsSameIdx(int idx);

private:
	UserTable m_userTable;				/// �� �� ���� ����Ʈ
	std::string m_name;					/// �� �̸�
	int m_roomIdx;						/// ���� ������ȣ
	int m_maxUser;						/// �� �ִ��ο� ��
	bool m_destroyed;					/// ������ �濡 �ڴʰ� �����ϴ� ���� �����ϴ� �÷���
	std::weak_ptr<Room> m_selfPtr;		/// ������ shared_ptr ȹ��� ������(�� ���� �� ���)

	static RoomManager* m_roomMgr;		/// �� ���� ȣ�� ������
};


/*
RoomManager
���� �����ϴ� �Ŵ��� ��ü.
����, ����, �˻�, ��� ���� ó���Ѵ�.
*/
class RoomManager
{
	typedef std::shared_ptr<Room> RoomPtr;
	typedef std::map<int, RoomPtr> RoomTable;

public:
	RoomManager();
	~RoomManager() {};

	///�ʱ�ȭ �Լ�.
	void Initialize();
	///���� ����. ���� : name - Room�� �̸�, ��ȯ : ������ Room�� ������
	RoomPtr CreateRoom(const std::string& name, int maxUser);
	///���� ����. ���� : idx - Room�� �ε���, ��ȯ : ��������
	bool DestroyRoom(int idx);
	///���� ������ ȹ��. ���� : idx - Room�� �ε���, ��ȯ : Room�� ������, ���� �� nullptr
	RoomPtr GetRoom(int idx);
	///��� Room�� ��� ȹ��. ��ȯ : ������ ���� �̸���
	std::string GetRoomList();

private:
	RoomTable m_roomTable;				/// ���� �����ϴ� ���̺�
	int m_genRoomCnt;					/// ������� ������ ���� ��
	std::stack<int> m_reuseRoomCnt;		/// �� �ε��� ���� ���� �����̳�
};