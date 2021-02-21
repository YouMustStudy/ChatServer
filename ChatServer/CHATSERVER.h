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
class CHATSERVER
{
	typedef std::map<SOCKET, User> UserTable;

public:

	CHATSERVER() :
		m_addr(),
		m_listener(INVALID_SOCKET),
		Lobby("Lobby")
	{};
	~CHATSERVER() {};

	bool Initialize(short port);
	void Run();
	void Terminate();

private:
	SOCKADDR_IN m_addr;
	SOCKET m_listener;
	UserTable m_userTable;
	Room Lobby;

	bool InitWSA(short port);

	void ProcessPacket(User& user, std::string data);
};