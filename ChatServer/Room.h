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
*유저들이 모이는 하나의 단위로 채팅은 해당 방 내에서만 이뤄진다.
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
	*@brief 방 내 모든 유저들에게 공지를 전송한다.
	*@param[in] msg 전송할 메세지.
	*/
	void NotifyAll(const std::string& msg);

	/**
	*@brief 방 내 모든 유저들에게 메세지를 전송한다.
	*@param[in] sender 전송자의 소켓.
	*@param[in] msg 전송할 메세지.
	*/
	void SendChat(SOCKET socket, const std::string& msg) const;

	/**
	*@brief 방 내 모든 유저들의 이름을 반환한다.
	*@return 유저들의 이름이 담긴 string 객체.
	*/
	std::string GetUserList();

private:
	/**
	*@brief 유저를 방에 입장시킨다.
	*@param[in] user 입장할 유저의 포인터.
	*@return 성공 시 true, 인원 제한에 걸려 입장을 못할 시 false.
	*/
	bool Enter(SOCKET socket, const std::string& name);

	/**
	*@brief 방에서 유저를 퇴장시킨다. 동시에 남은 인원에게 유저의 퇴장을 알린다. 남은 인원이 없을 시 방을 제거한다.
	*@param[in] uid 퇴장할 유저의 UID.
	*@return 방의 남은 인원.
	*/
	size_t Leave(SOCKET socket);

	UserList m_userList;						///< 방 내 유저 리스트
	UserTable m_userTable;						///< 방 내 유저 리스트
	std::string m_name;							///< 방 이름
	size_t m_roomIdx{0};						///< 방의 고유번호
	int m_maxUser{0};							///< 방 최대인원 수
};


/**
*@brief
*방을 관리하는 매니저 객체.
*방의 생성, 삭제, 검색, 목록 등을 처리한다.
*/
class RoomManager
{
	using RoomList = std::vector<Room>;
	using RoomTable = std::unordered_map<size_t, size_t>;

public:
	/**
	*@brief 방을 생성한다.
	*@param[in] name 방의 이름.
	*@param[in] maxUser 방의 최대 인원.
	*@return 생성된 방의 인덱스.
	*/
	int CreateRoom(const std::string& name, int maxUser, bool roomLimit = true);

	/**
	*@brief 방을 검색한다.
	*@param[in] idx 검색할 방의 인덱스.
	*@return 검색된 방의 포인터. 실패시 nullptr.
	*/
	Room* GetRoom(size_t idx);

	/**
	*@brief 방을 테이블에서 제거한다.
	*@param[in] idx 제거할 방의 인덱스.
	*/
	void DestroyRoom(int idx);

	/**
	*@brief 생성된 모든 방의 목록을 반환한다.
	*@return 방의 이름들이 담긴 string 객체.
	*/
	std::string GetRoomList();
	
	/**
	*@brief 싱글턴 호출용 함수. g_roomManager로 매크로가 되어있다.
	*@return 싱글턴 객체의 레퍼런스.
	*/
	static RoomManager& Instance();

	bool Enter(SOCKET socket, const std::string& name, int idx);
	bool Leave(SOCKET socket, size_t idx);

private:
	RoomTable m_roomTable;				///< 룸리스트와 방번호 간 매핑 테이블.
	RoomList m_roomList;				///< 실제 데이터가 저장된 룸 리스트.
	int m_genRoomCnt{0};				///< 현재까지 생성된 방의 수
	std::stack<int> m_reuseRoomCnt;		///< 방 인덱스 재사용 저장 컨테이너

	RoomManager() {};
};
//@brief RoomManager 호출 매크로.
#define g_roomManager (RoomManager::Instance())
