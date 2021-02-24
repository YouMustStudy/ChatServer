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
	typedef std::map<SOCKET, UserPtr> sessionTable;

public:
	ChatServer() :
		m_listener(INVALID_SOCKET),
		m_lobby(nullptr)
	{};
	~ChatServer() {};

	/**
	*@brief 서버를 초기화한다.
	*@param[in] 서버의 포트 번호.
	*@return 성공 시 true, 실패 시 false.
	*/
	bool Initialize(short port);

	/**
	*@brief Accept를 시작한다.
	*/
	void Run();

	/**
	*@brief 서버의 종료처리를 한다.
	*/
	void Terminate();

private:
	SOCKET m_listener;				///< Listen 소켓
	sessionTable m_sessionTable;	///< 세션 테이블(로그인 이전 유저를 포함하는 전체 테이블)
	RoomPtr m_lobby;				///< 로비의 포인터
	CmdParser m_cmdParser;			///< 명령어 처리객체

	/**
	*@brief WSA환경 초기화.
	*@param[in] 서버의 포트 번호.
	*@return 성공 시 true, 실패 시 false.
	*/
	bool InitWSA(short port);

	/**
	*@brief 로비를 생성한다.
	*@return 성공 시 true, 실패 시 false.
	*/
	bool InitLobby();

	/**
	*@brief 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*@param[in] data 처리할 데이터.
	*/
	void ProcessPacket(UserPtr& user, std::string data);

	/**
	*@brief 유저를 종료처리한다.
	*@param[in] user 종료할 유저의 포인터.
	*/
	void DisconnectUser(UserPtr& user);

	/**
	*@brief 유저를 기존 방에서 새로운 방으로 이동시킨다.
	*@param[in] user 요청한 유저의 포인터.
	*@param[in] enterRoom 새로 들어갈 방의 포인터.
	*/
	void ExchangeRoom(UserPtr& user, RoomPtr& enterRoom);

	/**
	*@brief 새로운 유저 접속 시 세션을 추가한다.
	*@param[in] socket 세션에서 사용할 소켓.
	*@return 생성된 세션의 UserPtr, 실패 시 nullptr.
	*/
	UserPtr AddSession(SOCKET socket);

	/**
	*@brief 접속 종료 시 세션을 삭제한다.
	*@param[in] socket 세션에서 사용하던 소켓.
	*@return 성공 시 1, 실패 시 0.
	*/
	size_t EraseSession(SOCKET socket);

	// 명령어 처리 함수들.
	/**
	*@brief 로그인 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*@param[in] userName 로그인할 이름.
	*/
	void ProcessLogin(UserPtr &user, const std::string& userName);

	/**
	*@brief 채팅 패킷을 처리한다.
	*@param[in] user 송신자의 포인터.
	*@param[in] msg 보낼 메세지.
	*/
	void ProcessChat(const UserPtr& sender, const std::string& msg);

	/**
	*@brief 입장 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*@param[in] userName 로그인할 방의 인덱스.
	*/
	void ProcessJoin(UserPtr& user, int roomIdx);

	/**
	*@brief 퇴장 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*/
	void ProcessQuit(UserPtr& user);

	/**
	*@brief 귓속말 패킷을 처리한다.
	*@param[in] sender 송신자의 포인터.
	*@param[in] receiverName 수신자의 이름.
	*@param[in] msg 보낼 메세지.
	*/
	void ProcessMsg(const UserPtr& sender, const std::string& receiverName, const std::string& msg);

	/**
	*@brief 현재 방 내 유저 목록 요청 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*/
	void ProcessGetUserList(const UserPtr& user);

	/**
	*@brief 방 목록 요청 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*/
	void ProcessGetRoomList(const UserPtr& user);

	/**
	*@brief 서버 내 유저 목록 요청 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*/
	void ProcessGetAllUserList(const UserPtr& user);

	/**
	*@brief 방 생성 요청 패킷을 처리한다. 유저는 방 생성 후 바로 해당 방으로 입장한다.
	*@param[in] user 요청한 유저의 포인터.
	*@param[in] roomName 생성할 방의 이름.
	*@param[in] maxUser 방의 인원 제한 수.
	*/
	void ProcessCreateRoom(UserPtr & user, const std::string& roomName, int maxUser);

	/**
	*@brief 도움말 요청 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*/
	void ProcessHelp(const UserPtr& user);

	/**
	*@brief 잘못된 명령어를 수신받았을 때의 처리함수. 유저에게 오류 메세지를 보낸다.
	*@param[in] user 요청한 유저의 포인터.
	*/
	void ProcessError(const UserPtr& user);
};