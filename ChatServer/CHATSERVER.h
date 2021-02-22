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

	/// ���� �ʱ�ȭ. ���� : ��Ʈ��ȣ
	bool Initialize(short port);
	/// Accept ����.
	void Run();
	/// ���� ����ó��
	void Terminate();

private:
	SOCKADDR_IN m_addr;			/// ???
	SOCKET m_listener;			/// Listen ����
	UserTable m_userTable;		/// ���� ���̺�
	RoomManager m_roomMgr;		/// �� �Ŵ���
	RoomPtr m_lobby;			/// �κ� ������

	/// WSA �ʱ�ȭ.
	bool InitWSA(short port);
	/// ��Ŷ ó��. ���� : user - ó���� ���� ������, data - ó���� ������
	void ProcessPacket(UserPtr& user, std::string data);
	/// ���� ���� ó��. ���� : user - ó���� ���� ������
	void DisconnectUser(UserPtr& user);
};