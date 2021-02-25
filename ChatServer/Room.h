#pragma once
#include <set>
#include <map>
#include <string>
#include <stack>
#include <memory>
#include <iostream>

#include "Config.h"

class User;
using UserPtr = std::shared_ptr<User>;

class RoomManager;
/**
*@brief 
*�������� ���̴� �ϳ��� ������ ä���� �ش� �� �������� �̷�����.
*/
class Room
{
	friend RoomManager;
	using UserTable = std::set<UserPtr>;
	using RoomPtr = std::shared_ptr<Room>;

public:
	Room(const std::string& name, int idx, int maxUser) : m_maxUser(maxUser), m_name(name), m_roomIdx(idx) {};

	/**
	*@brief �ڱ� �ڽ��� weak_ptr�� �����ϴ� �Լ�, ������ �濡 ���� �� ������ �� �����͸� ������ ������ �����ϱ� ���� �ʿ��ϴ�.
	*@param[in] myself �ڱ� �ڽ��� RoomPtr
	*/
	void SetWeakPtr(RoomPtr& myself);

	/**
	*@brief ������ �濡 �����Ų��.
	*@param[in] user ������ ������ ������.
	*@return ���� �� true, �ο� ���ѿ� �ɷ� ������ ���� �� false.
	*/
	bool Enter(UserPtr &user);

	/**
	*@brief �濡�� ������ �����Ų��. ���ÿ� ���� �ο����� ������ ������ �˸���. ���� �ο��� ���� �� ���� �����Ѵ�.
	*@param[in] user ������ ������ ������.
	*@return ���� �� true, ���� �� false.
	*/
	bool Leave(UserPtr &user);

	/**
	*@brief �� �� ��� �����鿡�� ������ �����Ѵ�.
	*@param[in] msg ������ �޼���.
	*/
	void NotifyAll(const std::string& msg);

	/**
	*@brief �� �� ��� �����鿡�� �޼����� �����Ѵ�.
	*@param[in] sender �������� ������.
	*@param[in] msg ������ �޼���.
	*/
	void SendChat(const UserPtr& sender, const std::string& msg);

	/**
	*@brief �� �� ��� �������� �̸��� ��ȯ�Ѵ�.
	*@return �������� �̸��� ��� string ��ü.
	*/
	std::string GetUserList();
	
	/**
	*@brief ������ �ε����� ���ڿ� ������ �� ���Ѵ�.
	*@param[in] idx �˻��� �ε���.
	*@return ���ٸ� true, �ٸ��� false.
	*/
	bool IsSameIdx(int idx);

private:
	UserTable m_userTable;						///< �� �� ���� ����Ʈ
	std::string m_name;							///< �� �̸�
	int m_roomIdx{-1};							///< ���� ������ȣ
	int m_maxUser{0};							///< �� �ִ��ο� ��
	bool m_destroyed{false};					///< ������ �濡 �ڴʰ� �����ϴ� ���� �����ϴ� �÷���
	std::weak_ptr<Room> m_selfPtr;				///< ������ shared_ptr ȹ��� ������(�� ���� �� ���)
};


/**
*@brief
*���� �����ϴ� �Ŵ��� ��ü.
*���� ����, ����, �˻�, ��� ���� ó���Ѵ�.
*/
class RoomManager
{
	using RoomPtr = std::shared_ptr<Room>;
	using RoomTable = std::map<int, RoomPtr>;

public:
	/**
	*@brief ���� ���̺� �����Ѵ�.
	*@param[in] name ������ ���� �̸�.
	*@param[in] maxUser �ִ� �ο� ���� ��.
	*@param[in] userLimit �ش� ���� ������ �� �ִ� �ο� ���� ������ ������ ����.
	*@return ������ ���� RoomPtr, ���� �� nullptr.
	*/
	RoomPtr CreateRoom(const std::string& name, int maxUser, bool userLimit = true);

	/**
	*@brief ���� ���̺��� �����Ѵ�.
	*@param[in] idx ������ ���� �ε���.
	*@return ���� �� true, ���� �� false.
	*/
	bool DestroyRoom(int idx);

	/**
	*@brief �ε����� ���� �˻�, �����͸� ��ȯ�Ѵ�.
	*@param[in] idx �˻��� ���� �ε���
	*@return ���� �� ���� RoomPtr, ���� �� nullptr.
	*/
	RoomPtr GetRoom(int idx);

	/**
	*@brief ������ ��� ���� ����� ��ȯ�Ѵ�.
	*@return ���� �̸����� ��� string ��ü.
	*/
	std::string GetRoomList();
	
	/**
	*@brief �̱��� ȣ��� �Լ�. g_roomManager�� ��ũ�ΰ� �Ǿ��ִ�.
	*@return �̱��� ��ü�� ���۷���.
	*/
	static RoomManager& Instance();

private:
	RoomTable m_roomTable;				///< ���� �����ϴ� ���̺�
	int m_genRoomCnt{0};				///< ������� ������ ���� ��
	std::stack<int> m_reuseRoomCnt;		///< �� �ε��� ���� ���� �����̳�

	RoomManager();
};

//@brief RoomManager ȣ�� ��ũ��.
#define g_roomManager (RoomManager::Instance())