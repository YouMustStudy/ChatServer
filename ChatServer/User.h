#pragma once
#include <ws2tcpip.h>
#include <string>
#include <unordered_map>
#include <memory>

class Room;
using RoomPtr = std::shared_ptr<Room>;
/**
*@brief 유저 데이터를 관리한다.
*/
class User
{
public:
	User(SOCKET socket, SOCKADDR_IN addr);
	~User() {};

	/**
	*@brief 유저에게 메세지를 전송한다.
	*@param[in] msg 전송할 메세지.
	*/
	void SendChat(const std::string& msg);

	/**
	*@brief 유저가 있는 방의 포인터를 반환한다.
	*@return 유저가 있는 방의 포인터, 없으면 nullptr.
	*/
	RoomPtr GetRoom() const;

	/**
	*@brief 유저가 있는 방의 포인터를 설정한다.
	*@param[in] roomPtr 유저가 있을 방의 포인터.
	*/
	void SetRoom(const RoomPtr &roomPtr);

	/**
	*@brief 유저 이름을 반환한다.
	*@return 유저 이름.
	*/
	std::string GetName() const;

	/**
	*@brief 유저의 이름을 설정한다.
	*@param[in] name 설정할 이름.
	*/
	void SetName(const std::string& name);
	
	/**
	*@brief 유저 소켓을 반환한다.
	*@return 유저 소켓.
	*/
	SOCKET GetSocket();

	/**
	*@brief 유저의 소켓을 설정한다.
	*@param[in] socket 설정할 소켓.
	*/
	void SetSocket(SOCKET socket);

	/**
	*@brief 유저 세션이 살아있는 지 확인한다.
	*@return 살아있다면 true, 죽었으면 false.
	*/
	bool IsAlive();

	/**
	*@brief 유저 소켓을 종료시킨 후 마킹한다.
	*/
	void Kill();

	/**
	*@brief 유저가 로그인했는 지 아닌지 알려준다.
	*@return 로그인했을 시 true, 아닐 시 false.
	*/
	bool GetIsLogin();
	void SetLogin(const std::string& name);

	/**
	*@brief 유저의 주소값을 반환한다.
	*@return [IP:PORT] 형식의 문자열.
	*/
	std::string GetAddr();

	/**
	*@brief 유저 버퍼에 데이터를 순차적으로 삽입한다. 백스페이스를 처리한다.
	*@param[in] data 입력할 데이터 버퍼.
	*@param[in] length 입력할 데이터의 길이.
	*/
	void PushData(const char* data, int length);
	std::string m_data;					///< 유저 버퍼, 'select thread'에서만 접근한다.
private:
	std::string m_name;					///< 유저 닉네임
	RoomPtr m_room{nullptr};			///< 입장한 방 포인터
	bool m_login{false};				///< 로그인 여부
	bool m_isAlive{true};				///< 세션 생존 여부
	SOCKET m_socket{INVALID_SOCKET};	///< 유저의 소켓
	std::string m_addr;					///< 유저의 주소 저장 - [IP:PORT]
	
};

/**
*breif '로그인한' 유저 관리 클래스.
*/
class UserManager
{
	using UserPtr = std::shared_ptr<User>;
	using UserTable = std::unordered_map<std::string, UserPtr>;

public:
	/**
	*@brief 싱글턴 호출용 함수. g_userManager로 매크로가 되어있다.
	*@return 싱글턴 객체의 레퍼런스.
	*/
	static UserManager& Instance();

	/**
	*@brief 유저의 이름을 검색, 포인터를 반환한다.
	*@param[in] userName 검색할 유저의 이름.
	*@return 성공 시 해당 유저의 포인터, 실패 시 nullptr.
	*/
	UserPtr GetUser(const std::string& userName);
	
	/**
	*@brief 유저를 테이블에 추가한다.
	*@param[in] user 추가할 유저의 포인터.
	*@param[in] userName 추가할 유저의 이름
	*@return 성공 시 true, 닉네임 중복 시 false.
	*/
	bool AddUser(UserPtr& user, const std::string& userName);

	/**
	*@brief 유저를 유저 테이블에서 삭제한다.
	*@param[in] user 삭제할 유저의 이름
	*@return 삭제된 객체의 수, 성공 시 1, 실패 시 0.
	*/
	size_t EraseUser(const UserPtr& user);
	
	/**
	*@brief 서버 내 모든 유저들의 이름을 반환한다.
	*@return 유저들의 이름이 담긴 string 객체.
	*/
	std::string GetUserList();

	/**
	*@brief 유저를 종료처리한다.
	*@param[in] user 종료할 유저의 이름.
	*/
	void DisconnectUser(const std::string& userName);

	/**
	*@brief 유저를 종료처리한다.
	*@param[in] user 종료할 유저의 포인터.
	*/
	void DisconnectUser(UserPtr& user);
private:
	UserManager() {};
	UserTable m_userTable;	///< 유저 저장용 컨테이너
};
//@brief UserManager 호출 매크로.
#define g_userManager (UserManager::Instance())