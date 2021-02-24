#pragma once
#include <WinSock2.h>
#include <string>
#include <map>
#include <memory>

class Room;
typedef std::shared_ptr<Room> RoomPtr;
/*
User
유저 데이터 관리 클래스
*/
class User
{
public:
	User() : m_socket(INVALID_SOCKET), m_name(), m_data(), m_room(nullptr), m_login(false) {};
	User(SOCKET socket) : m_socket(socket), m_name(), m_data(), m_room(nullptr), m_login(false) {};
	~User() {};
	void SendChat(const std::string& msg);

	///GetSets
	RoomPtr GetRoom();
	void SetRoom(const RoomPtr &roomPtr);

	void SetName(const std::string& name);
	std::string GetName();

	SOCKET GetSocket();
	void SetSocket(SOCKET socket);

	bool GetIsLogin();
	void SetLogin(const std::string& name);

	///Thread-safe하지않음. 싱글스레드에서만 호출할 것.
	void PushData(const char* data, int length);
	std::string m_data;
private:
	std::string m_name;		/// 유저 닉네임
	RoomPtr m_room;			/// 입장한 방 포인터
	SOCKET m_socket;
	bool m_login;
};

class UserManager
{
	typedef std::shared_ptr<User> UserPtr;
	typedef std::map<std::string, UserPtr> UserTable;

public:
	static UserManager& Instance();

	UserPtr GetUser(const std::string& userName);
	bool AddUser(UserPtr& user, const std::string& userName);
	size_t EraseUser(const UserPtr& user);
	std::string GetUserList();
private:
	UserManager();
	UserTable m_userTable;
};
#define g_userManager (UserManager::Instance())