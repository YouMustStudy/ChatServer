#pragma once
#pragma comment(lib, "ws2_32")

///NW Headers
#include <WinSock2.h>
#include <fcntl.h>

#include <iostream>
#include <string>
#include <map>
#include <assert.h>

#include "Room.h"
#include "User.h"

static constexpr int BUF_SIZE = 1024;
class ChatServer
{
	typedef std::map<SOCKET, UserPtr> UserTable;

public:

	ChatServer() :
		m_addr(),
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
	SOCKADDR_IN m_addr;			/// ???
	SOCKET m_listener;			/// Listen 소켓
	UserTable m_userTable;		/// 유저 테이블
	RoomManager m_roomMgr;		/// 룸 매니저
	RoomPtr m_lobby;			/// 로비 포인터

	/// WSA 초기화.
	bool InitWSA(short port);
	/// 패킷 처리. 인자 : user - 처리할 유저 포인터, data - 처리할 데이터
	void ProcessPacket(UserPtr& user, std::string data);
	/// 유저 종료 처리. 인자 : user - 처리할 유저 포인터
	void DisconnectUser(UserPtr& user);
};