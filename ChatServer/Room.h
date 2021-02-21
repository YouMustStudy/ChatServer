#pragma once
#include <set>
#include <string>

class User;

class Room
{
	typedef User* UserPtr;
	typedef std::set<UserPtr> UserList;

public:
	Room() : m_userList(), maxUser(INT_MAX), id() {};
	Room(const std::string& id) : m_userList(), maxUser(INT_MAX), id(id) {};
	~Room() {};

	bool Enter(UserPtr user);
	bool Leave(UserPtr user);

	void NotifyAll(const std::string& msg);
	void SendChat(const UserPtr sender, const std::string& msg);

private:
	int maxUser;
	UserList m_userList;
	std::string id;
};