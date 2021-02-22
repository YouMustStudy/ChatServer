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
class Room
{
	friend RoomManager;
	typedef std::set<UserPtr> UserList;

public:
	Room() : m_userList(), m_maxUser(INT_MAX), m_name(), m_roomIdx() {};
	Room(const std::string& name, int idx) : m_userList(), m_maxUser(INT_MAX), m_name(name), m_roomIdx(idx) {};
	~Room() {};

	bool Enter(UserPtr user);
	bool Leave(UserPtr user);

	void NotifyAll(const std::string& msg);
	void SendChat(const UserPtr sender, const std::string& msg);

	std::string GetUserList();
	
private:
	int m_maxUser;
	UserList m_userList;
	std::string m_name;
	int m_roomIdx;
	static RoomManager* m_roomMgr;
};

typedef std::shared_ptr<Room> RoomPtr;
class RoomManager
{
	typedef std::map<int, RoomPtr> RoomTable;

public:
	RoomManager();
	~RoomManager() {};

	void Initialize();
	RoomPtr CreateRoom(const std::string& name);
	bool DestroyRoom(int idx);
	RoomPtr GetRoom(int idx);
	std::string GetRoomList();

private:
	int m_genRoomCnt;
	RoomTable m_roomList;
	std::stack<int> m_reuseRoomCnt;
};

