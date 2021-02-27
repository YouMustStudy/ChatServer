#pragma once
#include <WS2tcpip.h>
#include <string>
#include <unordered_map>
class Session
{
public:
	SOCKET socket;
	std::string buffer;
	std::string addr;
};

using SessionTable = std::unordered_map<SOCKET, Session>;