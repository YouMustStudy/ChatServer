#pragma once
#include <ws2tcpip.h>
#include <string>
#include <unordered_map>
#include <memory>

class Room;
/**
*@brief 유저 데이터를 관리한다.
*/
class User
{
public:
	User(SOCKET soc, std::string addr);
	~User() {};

	/**
	*@brief 유저에게 메세지를 전송한다.
	*@param[in] msg 전송할 메세지.
	*/
	void SendChat(const std::string& msg);

public:
	std::string m_name;					///< 유저 이름
	size_t m_room;						///< 입장한 방번호
	bool m_login{false};				///< 로그인 여부
	SOCKET m_socket;					///< 유저의 소켓
	std::string m_addr;					///< 유저의 주소 저장 - [IP:PORT]
	int m_refCnt{ 1 };					///< 현재 참조당하고 있는 갯수, 접속, send 시 증가하고, send 종료, 종료 시 감소한다.
};

/**
*breif 유저 관리 클래스.
*/
class UserManager
{
	using UserTable = std::unordered_map<std::string, size_t>;
	using UserList = std::vector<User>;
	using SocketTable = std::unordered_map<SOCKET, size_t>;

public:
	/**
	*@brief 싱글턴 호출용 함수. g_userManager로 매크로가 되어있다.
	*@return 싱글턴 객체의 레퍼런스.
	*/
	static UserManager& Instance();

	/**
	*@brief 유저를 테이블에 추가한다.
	*@param[in] user 추가할 유저의 포인터.
	*@param[in] userName 추가할 유저의 이름
	*/
	void AddUser(SOCKET socket, const std::string& addr);

	/**
	*@brief 유저의 참조 카운트를 감소. send 완료 혹은 유저의 종료 시 호출된다. 참조카운트가 0이 되면 객체를 삭제한다.
	*@param[in] socket 감소시킬 유저의 소켓.
	*/
	void DecreaseUser(SOCKET socket);
	
	/**
	*@brief 유저를 로그인 시킨다. 입력한 이름이 중복일 경우 false 리턴.
	*@param[in] socket 로그인 요청을 유저의 소켓.
	*@param[in] name 유저의 이름.
	*@return 성공 시 true, 입력한 이름이 중복일 경우 false 리턴.
	*/
	bool Login(SOCKET socket, const std::string& name);

	/**
	*@brief 서버 내 모든 유저들의 이름을 반환한다.
	*@return 유저들의 이름이 담긴 string 객체.
	*/
	std::string GetUserList();

	/**
	*@brief 소켓과 매칭되는 유저의 포인터 반환.
	*@param[in] socket 검색할 유저의 소켓.
	*@return 유저포인터, 검색 실패 시 nullptr.
	*/
	User* GetUser(SOCKET socket);

	/**
	*@brief 이름과 매칭되는 유저의 포인터 반환.
	*@param[in] socket 검색할 유저의 소켓.
	*@return 유저포인터, 검색 실패 시 nullptr.
	*/
	User* GetUser(const std::string& name);

	/**
	*@brief 소켓과 매핑되는 유저에게 메세지 전송.
	*@param[in] socket 전송할 유저의 소켓.
	*@param[in] msg 전송할 문자열
	*/
	void SendMsg(SOCKET socket, const std::string& msg);

public:
	friend class UserProcesser;
private:
	void DisconnectUser(SOCKET socket);

	UserManager() {};
	UserTable m_userTable;										///< 이름과 인덱스를 매핑하는 테이블.
	UserList m_userList;										///< 유저 실저장용 컨테이너.
	SocketTable m_SocketTable;									///< 소켓과 유저를 매핑하는 테이블
};
//@brief UserManager 호출 매크로.
#define g_userManager (UserManager::Instance())