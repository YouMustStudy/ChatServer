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

/*
User
'로그인한' 유저 관리 클래스
*/
class UserManager
{
	typedef std::shared_ptr<User> UserPtr;
	typedef std::map<std::string, UserPtr> UserTable;

public:
	static UserManager& Instance();
	///이름으로 유저의 포인터를 획득하는 함수. 인자 : userName - 검색할 유저 이름, 반환 : 검색된 유저의 포인터, 실패시 nullptr 반환
	UserPtr GetUser(const std::string& userName);
	///목록에 유저 추가. 인자 : user - 추가될 유저의 포인터, userName - 추가될 유저의 이름, 반환값 : 성공여부, 이름 중복 시 false 반환
	bool AddUser(UserPtr& user, const std::string& userName);
	///목록에서 유저 삭제. 인자 : user - 삭제될 유저의 포인터, 반환 : 삭제된 수
	size_t EraseUser(const UserPtr& user);
	///로그인한 모든 유저의 목록 반환.
	std::string GetUserList();
private:
	UserManager();
	UserTable m_userTable;			///유저 저장용 컨테이너
};
#define g_userManager (UserManager::Instance())