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
/*
Room
유저들이 모이는 하나의 단위
채팅은 해당 방 내에서만 이뤄진다.
*/
class Room
{
	friend RoomManager;
	typedef std::shared_ptr<Room> RoomPtr;
	typedef std::set<UserPtr> UserTable;

public:
	Room() : m_userTable(), m_maxUser(INT_MAX), m_name(), m_roomIdx(), m_destroyed(false) {};
	Room(const std::string& name, int idx) : m_userTable(), m_maxUser(INT_MAX), m_name(name), m_roomIdx(idx), m_destroyed(false) {};
	~Room() {};

	/// 자기 자신의 weak_ptr을 저장하는 함수, 인자 : myself - 본인의 shared_ptr
	void SetWeakPtr(RoomPtr& myself);
	///유저를 방에 입장. 인자 : user - 입장할 유저의 포인터, 반환 : 성공여부
	bool Enter(UserPtr user);
	///유저를 방에서 퇴장. 인자 : user - 입장할 유저의 포인터, 반환 : 성공여부
	bool Leave(const UserPtr user);
	///방 내의 모든 유저들에게 공지 전송. 인자 : msg - 보낼 메세지
	void NotifyAll(const std::string& msg);
	///방 내의 모든 유저들에게 유저 메세지 전송. 인자 : sender - 보내는 사람, msg - 보낼 메세지
	void SendChat(const UserPtr sender, const std::string& msg);
	///방 내의 모든 유저 이름 반환. 반환 : 방 내의 유저 이름들
	std::string GetUserList();

private:
	UserTable m_userTable;				/// 방 내 유저 리스트
	std::string m_name;					/// 방 이름
	int m_roomIdx;						/// 방의 고유번호
	int m_maxUser;						/// 방 최대인원 수
	bool m_destroyed;					/// 삭제된 방에 뒤늦게 입장하는 것을 방지하는 플래그
	std::weak_ptr<Room> m_selfPtr;		/// 본인의 shared_ptr 획득용 포인터(방 입장 시 사용)

	static RoomManager* m_roomMgr;		/// 방 삭제 호출 포인터
};


/*
RoomManager
방을 관리하는 매니저 객체.
생성, 삭제, 검색, 목록 등을 처리한다.
*/
typedef std::shared_ptr<Room> RoomPtr;
class RoomManager
{
	typedef std::map<int, RoomPtr> RoomTable;

public:
	RoomManager();
	~RoomManager() {};

	///초기화 함수.
	void Initialize();
	///방을 생성. 인자 : name - Room의 이름, 반환 : 생성된 Room의 포인터
	RoomPtr CreateRoom(const std::string& name);
	///방을 삭제. 인자 : idx - Room의 인덱스, 반환 : 성공여부
	bool DestroyRoom(int idx);
	///방의 포인터 획득. 인자 : idx - Room의 인덱스, 반환 : Room의 포인터, 실패 시 nullptr
	RoomPtr GetRoom(int idx);
	///모든 Room의 목록 획득. 반환 : 생성된 방의 이름들
	std::string GetRoomList();

private:
	RoomTable m_roomTable;				/// 방을 관리하는 테이블
	int m_genRoomCnt;					/// 현재까지 생성된 방의 수
	std::stack<int> m_reuseRoomCnt;		/// 방 인덱스 재사용 저장 컨테이너
};