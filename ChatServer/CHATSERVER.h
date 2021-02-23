#pragma once
#pragma comment(lib, "ws2_32")

///NW Headers
#include <WinSock2.h>
#include <fcntl.h>

#include <iostream>
#include <string>
#include <map>
#include <deque>
#include <assert.h>
#include <algorithm>

#include "Room.h"
#include "User.h"
#include "CmdParser.h"

static constexpr int BUF_SIZE = 1024;
class ChatServer
{
	typedef std::map<SOCKET, UserPtr> UserTable;

public:

	ChatServer() :
		m_listener(INVALID_SOCKET),
		m_lobby(nullptr)
	{};
	~ChatServer() {};

	/// 서버 초기화. 인자 : 포트번호
	bool Initialize(short port);
	/// Accept 시작.
	void Run();
	/// 서버 종료처리
	void Terminate();

private:
	SOCKET m_listener;			/// Listen 소켓
	UserTable m_userTable;		/// 유저 테이블
	RoomManager m_roomMgr;		/// 룸 매니저
	RoomPtr m_lobby;			/// 로비 포인터
	CmdParser m_cmdParser;		/// 명령어 처리객체

	/// WSA 초기화.
	bool InitWSA(short port);
	/// 패킷 처리. 인자 : user - 요청한 유저 포인터, data - 처리할 데이터
	void ProcessPacket(UserPtr& user, std::string data);
	/// 유저 종료 처리. 인자 : user - 요청한 유저 포인터
	void DisconnectUser(UserPtr& user);
	/// 방 교환용 함수. 인자 : user - 요청한 유저 포인터, enterRoom - 새로 입장할 방 포인터
	void ExchangeRoom(UserPtr& user, RoomPtr& enterRoom);

	/// 명령어 처리 함수들.
	/// 채팅 명령 처리. 인자 : sender - 요청 유저의 포인터, msg - 보낼 메세지
	void ProcessChat(const UserPtr& sender, const std::string& msg);
	/// 방 입장 처리. 인자 : user - 입장할 유저의 포인터, roomIdx - 입장할 방의 인덱스(RoomManager의 Key)
	void ProcessJoin(UserPtr& user, int roomIdx);
	/// 방 퇴장 처리. 인자 : user - 방에서 나갈 유저의 포인터
	void ProcessQuit(UserPtr& user);
	/// 쪽지 명령 처리. 인자 : sender - 보낸 유저의 포인터, receiver - 받는 유저의 포인터, msg - 보낼 메세지
	void ProcessMsg(const UserPtr& sender, const UserPtr& receiver, const std::string& msg);
	/// 쪽지 명령 처리. 인자 : sender - 보낸 유저의 포인터, receiver - 받는 유저의 이름, msg - 보낼 메세지
	void ProcessMsg(const UserPtr& sender, const std::string& receiverName, const std::string& msg);
	/// 방 내부 인원의 목록 전송. 인자 : user - 요청 유저의 포인터
	void ProcessGetUserList(const UserPtr& user);
	/// 서버 내 방 목록 전송. 인자 : user - 요청 유저의 포인터
	void ProcessGetRoomList(const UserPtr& user);
	/// 도움 메세지 전송. user- 요청 유저의 포인터
	void ProcessHelp(const UserPtr& user);
	/// 잘못된 명령어 메세지 수신 시 핸들링. user- 요청 유저의 포인터
	void ProcessError(const UserPtr& user);
};