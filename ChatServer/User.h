#pragma once
#include <WinSock2.h>
#include <string>

class Room;
class User
{
	typedef Room* RoomPtr;

public:
	User() : m_socket(INVALID_SOCKET), id(), data(), m_Room(nullptr) {};
	User(SOCKET socket) : m_socket(socket), id(), data(), m_Room(nullptr) {};
	~User() {};

public:
	std::string id;
	RoomPtr m_Room;
	SOCKET m_socket;
	std::string data;
};