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
	*@brief ������ �ʱ�ȭ�Ѵ�.
	*@param[in] ������ ��Ʈ ��ȣ.
	*@return ���� �� true, ���� �� false.
	*/
	bool Initialize(short port);

	/**
	*@brief Accept�� �����Ѵ�.
	*/
	void Run();

	/**
	*@brief ������ ����ó���� �Ѵ�.
	*/
	void Terminate();

private:
	SOCKET m_listener;				///< Listen ����
	sessionTable m_sessionTable;	///< ���� ���̺�(�α��� ���� ������ �����ϴ� ��ü ���̺�)
	RoomPtr m_lobby;				///< �κ��� ������
	CmdParser m_cmdParser;			///< ��ɾ� ó����ü

	/**
	*@brief WSAȯ�� �ʱ�ȭ.
	*@param[in] ������ ��Ʈ ��ȣ.
	*@return ���� �� true, ���� �� false.
	*/
	bool InitWSA(short port);

	/**
	*@brief �κ� �����Ѵ�.
	*@return ���� �� true, ���� �� false.
	*/
	bool InitLobby();

	/**
	*@brief ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*@param[in] data ó���� ������.
	*/
	void ProcessPacket(UserPtr& user, std::string data);

	/**
	*@brief ������ ����ó���Ѵ�.
	*@param[in] user ������ ������ ������.
	*/
	void DisconnectUser(UserPtr& user);

	/**
	*@brief ������ ���� �濡�� ���ο� ������ �̵���Ų��.
	*@param[in] user ��û�� ������ ������.
	*@param[in] enterRoom ���� �� ���� ������.
	*/
	void ExchangeRoom(UserPtr& user, RoomPtr& enterRoom);

	/**
	*@brief ���ο� ���� ���� �� ������ �߰��Ѵ�.
	*@param[in] socket ���ǿ��� ����� ����.
	*@return ������ ������ UserPtr, ���� �� nullptr.
	*/
	UserPtr AddSession(SOCKET socket);

	/**
	*@brief ���� ���� �� ������ �����Ѵ�.
	*@param[in] socket ���ǿ��� ����ϴ� ����.
	*@return ���� �� 1, ���� �� 0.
	*/
	size_t EraseSession(SOCKET socket);

	// ��ɾ� ó�� �Լ���.
	/**
	*@brief �α��� ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*@param[in] userName �α����� �̸�.
	*/
	void ProcessLogin(UserPtr &user, const std::string& userName);

	/**
	*@brief ä�� ��Ŷ�� ó���Ѵ�.
	*@param[in] user �۽����� ������.
	*@param[in] msg ���� �޼���.
	*/
	void ProcessChat(const UserPtr& sender, const std::string& msg);

	/**
	*@brief ���� ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*@param[in] userName �α����� ���� �ε���.
	*/
	void ProcessJoin(UserPtr& user, int roomIdx);

	/**
	*@brief ���� ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*/
	void ProcessQuit(UserPtr& user);

	/**
	*@brief �ӼӸ� ��Ŷ�� ó���Ѵ�.
	*@param[in] sender �۽����� ������.
	*@param[in] receiverName �������� �̸�.
	*@param[in] msg ���� �޼���.
	*/
	void ProcessMsg(const UserPtr& sender, const std::string& receiverName, const std::string& msg);

	/**
	*@brief ���� �� �� ���� ��� ��û ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*/
	void ProcessGetUserList(const UserPtr& user);

	/**
	*@brief �� ��� ��û ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*/
	void ProcessGetRoomList(const UserPtr& user);

	/**
	*@brief ���� �� ���� ��� ��û ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*/
	void ProcessGetAllUserList(const UserPtr& user);

	/**
	*@brief �� ���� ��û ��Ŷ�� ó���Ѵ�. ������ �� ���� �� �ٷ� �ش� ������ �����Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*@param[in] roomName ������ ���� �̸�.
	*@param[in] maxUser ���� �ο� ���� ��.
	*/
	void ProcessCreateRoom(UserPtr & user, const std::string& roomName, int maxUser);

	/**
	*@brief ���� ��û ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*/
	void ProcessHelp(const UserPtr& user);

	/**
	*@brief �߸��� ��ɾ ���Ź޾��� ���� ó���Լ�. �������� ���� �޼����� ������.
	*@param[in] user ��û�� ������ ������.
	*/
	void ProcessError(const UserPtr& user);
};