#pragma once
#include <ws2tcpip.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <stack>
#include <iostream>
#include "Config.h"

class User;
/**
*@brief 
*�������� ���̴� �ϳ��� ������ ä���� �ش� �� �������� �̷�����.
*/
class Room
{
	friend class RoomManager;
	typedef struct UserInfo {
		SOCKET socket;
		std::string name;
	} UserInfo;
	using UserList = std::vector<UserInfo>;
	using UserTable = std::unordered_map<SOCKET, size_t>;

public:
	Room(const std::string& name, int idx, int maxUser);

	/**
	*@brief �� �� ��� �����鿡�� ������ �����Ѵ�.
	*@param[in] msg ������ �޼���.
	*/
	void NotifyAll(const std::string& msg);

	/**
	*@brief �� �� ��� �����鿡�� �޼����� �����Ѵ�.
	*@param[in] sender �������� ����.
	*@param[in] msg ������ �޼���.
	*/
	void SendChat(SOCKET socket, const std::string& msg) const;

	/**
	*@brief �� �� ��� �������� �̸��� ��ȯ�Ѵ�.
	*@return �������� �̸��� ��� string ��ü.
	*/
	std::string GetUserList();

private:
	/**
	*@brief ������ �濡 �����Ų��.
	*@param[in] user ������ ������ ������.
	*@return ���� �� true, �ο� ���ѿ� �ɷ� ������ ���� �� false.
	*/
	bool Enter(SOCKET socket, const std::string& name);

	/**
	*@brief �濡�� ������ �����Ų��. ���ÿ� ���� �ο����� ������ ������ �˸���. ���� �ο��� ���� �� ���� �����Ѵ�.
	*@param[in] uid ������ ������ UID.
	*@return ���� ���� �ο�.
	*/
	size_t Leave(SOCKET socket);

	UserList m_userList;						///< �� �� ���� ����Ʈ
	UserTable m_userTable;						///< �� �� ���� ����Ʈ
	std::string m_name;							///< �� �̸�
	size_t m_roomIdx{0};						///< ���� ������ȣ
	int m_maxUser{0};							///< �� �ִ��ο� ��
};


/**
*@brief
*���� �����ϴ� �Ŵ��� ��ü.
*���� ����, ����, �˻�, ��� ���� ó���Ѵ�.
*/
class RoomManager
{
	using RoomList = std::vector<Room>;
	using RoomTable = std::unordered_map<size_t, size_t>;

public:
	/**
	*@brief ���� �����Ѵ�.
	*@param[in] name ���� �̸�.
	*@param[in] maxUser ���� �ִ� �ο�.
	*@return ������ ���� �ε���.
	*/
	int CreateRoom(const std::string& name, int maxUser, bool roomLimit = true);

	/**
	*@brief ���� �˻��Ѵ�.
	*@param[in] idx �˻��� ���� �ε���.
	*@return �˻��� ���� ������. ���н� nullptr.
	*/
	Room* GetRoom(size_t idx);

	/**
	*@brief ���� ���̺��� �����Ѵ�.
	*@param[in] idx ������ ���� �ε���.
	*/
	void DestroyRoom(int idx);

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

	bool Enter(SOCKET socket, const std::string& name, int idx);
	bool Leave(SOCKET socket, size_t idx);

private:
	RoomTable m_roomTable;				///< �븮��Ʈ�� ���ȣ �� ���� ���̺�.
	RoomList m_roomList;				///< ���� �����Ͱ� ����� �� ����Ʈ.
	int m_genRoomCnt{0};				///< ������� ������ ���� ��
	std::stack<int> m_reuseRoomCnt;		///< �� �ε��� ���� ���� �����̳�

	RoomManager() {};
};
//@brief RoomManager ȣ�� ��ũ��.
#define g_roomManager (RoomManager::Instance())
