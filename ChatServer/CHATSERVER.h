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

	/// ���� �ʱ�ȭ. ���� : ��Ʈ��ȣ
	bool Initialize(short port);
	/// Accept ����.
	void Run();
	/// ���� ����ó��
	void Terminate();

private:
	SOCKET m_listener;			/// Listen ����
	sessionTable m_sessionTable;/// ���� ���̺�(�α��� ���� ������ �����ϴ� ��ü ���̺�)
	RoomPtr m_lobby;			/// �κ� ������
	CmdParser m_cmdParser;		/// ��ɾ� ó����ü

	/// WSA �ʱ�ȭ.
	bool InitWSA(short port);
	/// ��Ŷ ó��. ���� : user - ��û�� ���� ������, data - ó���� ������
	void ProcessPacket(UserPtr& user, std::string data);
	/// ���� ���� ó��. ���� : user - ��û�� ���� ������
	void DisconnectUser(UserPtr& user);
	/// �� ��ȯ�� �Լ�. ���� : user - ��û�� ���� ������, enterRoom - ���� ������ �� ������
	void ExchangeRoom(UserPtr& user, RoomPtr& enterRoom);
	/// ���� ���� �� ������ �߰��ϴ� �Լ�. ���� : socket - ������ ���ϰ�, ��ȯ : ������ ������ ������
	UserPtr AddSession(SOCKET socket);
	/// ���� ���� �� ������ �����ϴ� �Լ�. ���� : socket - ������ ���ϰ�, ��ȯ : ������ ������ ��
	size_t EraseSession(SOCKET socket);
	

	/// ��ɾ� ó�� �Լ���.
	/// ���� �α��� ó�� �Լ�. ���� : user - ��û�� ���� ������, userName - �α����� ������ �̸�, ��ȯ : ��������
	bool ProcessLogin(UserPtr &user, const std::string& userName);
	/// ä�� ��� ó��. ���� : sender - ��û ������ ������, msg - ���� �޼���
	void ProcessChat(const UserPtr& sender, const std::string& msg);
	/// �� ���� ó��. ���� : user - ������ ������ ������, roomIdx - ������ ���� �ε���(RoomManager�� Key)
	void ProcessJoin(UserPtr& user, int roomIdx);
	/// �� ���� ó��. ���� : user - �濡�� ���� ������ ������
	void ProcessQuit(UserPtr& user);
	/// ���� ��� ó��. ���� : sender - ���� ������ ������, receiver - �޴� ������ �̸�, msg - ���� �޼���
	void ProcessMsg(const UserPtr& sender, const std::string& receiverName, const std::string& msg);
	/// �� ���� �ο��� ��� ����. ���� : user - ��û ������ ������
	void ProcessGetUserList(const UserPtr& user);
	/// ���� �� �� ��� ����. ���� : user - ��û ������ ������
	void ProcessGetRoomList(const UserPtr& user);
	/// ���� ������ �� ������ �ش� ������ �����Ų��. ���� : user - ��û ������ ������, roomName - ������ ���� �̸�, maxUser - ���� �ִ��ο� ��, ��ȯ : ������ ���� ������
	void ProcessCreateRoom(UserPtr & user, const std::string& roomName, int maxUser);
	/// ���� �޼��� ����. user- ��û ������ ������
	void ProcessHelp(const UserPtr& user);
	/// �߸��� ��ɾ� �޼��� ���� �� �ڵ鸵. user- ��û ������ ������
	void ProcessError(const UserPtr& user);
};