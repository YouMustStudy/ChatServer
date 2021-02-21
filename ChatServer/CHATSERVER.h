#pragma once
#pragma comment(lib, "ws2_32")

///NW Headers
#include <WinSock2.h>
#include <fcntl.h>

#include <iostream>
#include <string>
#include <assert.h>

static constexpr int BUF_SIZE = 1024;
class CHATSERVER
{
public:

	CHATSERVER() :
		m_addr(),
		m_listener(INVALID_SOCKET)
	{};
	~CHATSERVER() {};

	bool Initialize(short port);
	void Run();
	void Terminate();

private:
	SOCKADDR_IN m_addr;
	SOCKET m_listener;

	bool InitWSA(short port);

	void ProcessPacket(int client, std::string data);
};