#pragma once
#include <WinSock2.h>
#include <string>

class User
{
public:
	User() : m_socket(INVALID_SOCKET), id(), data() {};
	User(SOCKET socket) : m_socket(socket), id(), data() {};
	~User() {};

	bool operator<(const User& other) { return this->m_socket < other.m_socket; };
public:
	SOCKET m_socket;
	std::string id;
	std::string data;
};