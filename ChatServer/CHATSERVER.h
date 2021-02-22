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

	bool Initialize(short port);
	void Run();
	void Terminate();

private:
	SOCKADDR_IN m_addr;
	SOCKET m_listener;
	UserTable m_userTable;
	RoomManager m_roomMgr;
	RoomPtr m_lobby;

	bool InitWSA(short port);

	void ProcessPacket(UserPtr& user, std::string data);

	void DisconnectUser(UserPtr& user);
};