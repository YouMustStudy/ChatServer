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
*유저들이 모이는 하나의 단위로 채팅은 해당 방 내에서만 이뤄진다.
*/
class Room
{
	friend RoomManager;
	using UserTable = std::set<UserPtr>;
	using RoomPtr = std::shared_ptr<Room>;

public:
	Room(const std::string& name, int idx, int maxUser) : m_maxUser(maxUser), m_name(name), m_roomIdx(idx) {};

	/**
	*@brief 자기 자신의 weak_ptr을 저장하는 함수, 유저가 방에 입장 시 유저의 방 포인터를 본인의 것으로 수정하기 위해 필요하다.
	*@param[in] myself 자기 자신의 RoomPtr
	*/
	void SetWeakPtr(RoomPtr& myself);

	/**
	*@brief 유저를 방에 입장시킨다.
	*@param[in] user 입장할 유저의 포인터.
	*@return 성공 시 true, 인원 제한에 걸려 입장을 못할 시 false.
	*/
	bool Enter(UserPtr &user);

	/**
	*@brief 방에서 유저를 퇴장시킨다. 동시에 남은 인원에게 유저의 퇴장을 알린다. 남은 인원이 없을 시 방을 제거한다.
	*@param[in] user 퇴장할 유저의 포인터.
	*@return 성공 시 true, 실패 시 false.
	*/
	bool Leave(UserPtr &user);

	/**
	*@brief 방 내 모든 유저들에게 공지를 전송한다.
	*@param[in] msg 전송할 메세지.
	*/
	void NotifyAll(const std::string& msg);

	/**
	*@brief 방 내 모든 유저들에게 메세지를 전송한다.
	*@param[in] sender 전송자의 포인터.
	*@param[in] msg 전송할 메세지.
	*/
	void SendChat(const UserPtr& sender, const std::string& msg);

	/**
	*@brief 방 내 모든 유저들의 이름을 반환한다.
	*@return 유저들의 이름이 담긴 string 객체.
	*/
	std::string GetUserList();
	
	/**
	*@brief 본인의 인덱스가 인자와 동일한 지 비교한다.
	*@param[in] idx 검사할 인덱스.
	*@return 같다면 true, 다르면 false.
	*/
	bool IsSameIdx(int idx);

private:
	UserTable m_userTable;						///< 방 내 유저 리스트
	std::string m_name;							///< 방 이름
	int m_roomIdx{-1};							///< 방의 고유번호
	int m_maxUser{0};							///< 방 최대인원 수
	bool m_destroyed{false};					///< 삭제된 방에 뒤늦게 입장하는 것을 방지하는 플래그
	std::weak_ptr<Room> m_selfPtr;				///< 본인의 shared_ptr 획득용 포인터(방 입장 시 사용)
};


/**
*@brief
*방을 관리하는 매니저 객체.
*방의 생성, 삭제, 검색, 목록 등을 처리한다.
*/
class RoomManager
{
	using RoomPtr = std::shared_ptr<Room>;
	using RoomTable = std::map<int, RoomPtr>;

public:
	/**
	*@brief 방을 테이블에 생성한다.
	*@param[in] name 생성할 방의 이름.
	*@param[in] maxUser 최대 인원 제한 수.
	*@param[in] userLimit 해당 방을 생성할 때 최대 인원 수를 제한할 것인지 여부.
	*@return 생성된 방의 RoomPtr, 실패 시 nullptr.
	*/
	RoomPtr CreateRoom(const std::string& name, int maxUser, bool userLimit = true);

	/**
	*@brief 방을 테이블에서 제거한다.
	*@param[in] idx 제거할 방의 인덱스.
	*@return 성공 시 true, 실패 시 false.
	*/
	bool DestroyRoom(int idx);

	/**
	*@brief 인덱스로 방을 검색, 포인터를 반환한다.
	*@param[in] idx 검색할 방의 인덱스
	*@return 성공 시 방의 RoomPtr, 실패 시 nullptr.
	*/
	RoomPtr GetRoom(int idx);

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

private:
	RoomTable m_roomTable;				///< 방을 관리하는 테이블
	int m_genRoomCnt{0};				///< 현재까지 생성된 방의 수
	std::stack<int> m_reuseRoomCnt;		///< 방 인덱스 재사용 저장 컨테이너

	RoomManager();
};

//@brief RoomManager 호출 매크로.
#define g_roomManager (RoomManager::Instance())