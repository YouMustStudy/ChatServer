#pragma once
#include <WinSock2.h>
#include <string>
#include <memory>

class Room;
typedef std::shared_ptr<Room> RoomPtr;
/*
User
유저 데이터 관리 클래스
*/
class User
{
public:
	User() : m_socket(INVALID_SOCKET), m_name(), m_data(), m_room(nullptr) {};
	User(SOCKET socket) : m_socket(socket), m_name(), m_data(), m_room(nullptr) {};
	~User() {};
	void SendChat(const std::string& msg);

	RoomPtr GetRoom();
	void SetRoom(const RoomPtr &roomPtr);
	void SetName(std::string& name);
	SOCKET GetSocket();
	void SetSocket(SOCKET socket);
	void PushData(const char* data, int length);
	std::string GetName();
	
	std::string m_data;
private:
	std::string m_name;		/// 유저 닉네임
	RoomPtr m_room;			/// 입장한 방 포인터
	SOCKET m_socket;
};

