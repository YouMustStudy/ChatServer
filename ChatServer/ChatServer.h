#pragma once
#pragma comment(lib, "ws2_32")

///NW Headers
#include <ws2tcpip.h>
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
#include "Config.h"
#include "Logger.h"
#include "UtilFunc.h"

///MultiThread
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <concurrent_queue.h>
#include "Job.h"

enum SESSION
{
	SE_BUFFER,
	SE_ADDR,
	SE_COUNT
};

class ChatServer
{
	using SessionTable = std::map<SOCKET, std::string[SE_COUNT]>;

public:
	static ChatServer& Instance();

	/**
	*@brief ������ �ʱ�ȭ�Ѵ�.
	*@param[in] ������ ��Ʈ ��ȣ.
	*@return ���� �� true, ���� �� false.
	*/
	bool Initialize(short port = SERVER_PORT);

	/**
	*@brief Accept�� �����Ѵ�.
	*/
	void Run();

	/**
	*@brief ������ ����ó���� �Ѵ�.
	*/
	void Terminate();

private:
	SOCKET m_listener{INVALID_SOCKET};				///< Listen ����
	CmdParser m_cmdParser;							///< ��ɾ� ó����ü

	ChatServer() {};

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
	*@param[in] user ��û�� ������ ����.
	*@param[in] data ó���� ������.
	*/
	void ProcessPacket(const UserJob* jobPtr);

	// ��ɾ� ó�� �Լ���.
	/**
	*@brief �α��� ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*@param[in] userName �α����� �̸�.
	*/
	void ProcessLogin(User* user, const std::string& userName);

	/**
	*@brief ä�� ��Ŷ�� ó���Ѵ�.
	*@param[in] user �۽����� ������.
	*@param[in] msg ���� �޼���.
	*/
	void ProcessChat(User* sender, const std::string& msg);

	/**
	*@brief ���� ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*@param[in] userName �α����� ���� �ε���.
	*/
	void ProcessJoin(User* user, int roomIdx);

	/**
	*@brief ���� ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*/
	void ProcessQuit(User* user);

	/**
	*@brief �ӼӸ� ��Ŷ�� ó���Ѵ�.
	*@param[in] sender �۽����� ������.
	*@param[in] receiverName �������� �̸�.
	*@param[in] msg ���� �޼���.
	*/
	void ProcessMsg(User* sender, const std::string& receiverName, const std::string& msg);

	/**
	*@brief ���� �� �� ���� ��� ��û ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*/
	void ProcessGetUserList(User* user);

	/**
	*@brief �� ��� ��û ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*/
	void ProcessGetRoomList(User* user);

	/**
	*@brief ���� �� ���� ��� ��û ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*/
	void ProcessGetAllUserList(User* user);

	/**
	*@brief �� ���� ��û ��Ŷ�� ó���Ѵ�. ������ �� ���� �� �ٷ� �ش� ������ �����Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*@param[in] roomName ������ ���� �̸�.
	*@param[in] maxUser ���� �ο� ���� ��.
	*/
	void ProcessCreateRoom(User* user, const std::string& roomName, int maxUser);

	/**
	*@brief ���� ��û ��Ŷ�� ó���Ѵ�.
	*@param[in] user ��û�� ������ ������.
	*/
	void ProcessHelp(User* user);

	/**
	*@brief �߸��� ��ɾ ���Ź޾��� ���� ó���Լ�. �������� ���� �޼����� ������.
	*@param[in] user ��û�� ������ ������.
	*/
	void ProcessError(User* user);


	///select ������� IOCP�� ����� ���� �����ϴ� ���� ��ǥ�� �Ѵ�.
	///APC Queue�� PPL�� concurrent_queue�� ���.
	///GSCQ�� c++�� condition_variable�� ��ü�Ѵ�.
	///MultiThread.
	concurrency::concurrent_queue<MainJob*> m_workerQueue;	/// �۾��� �۾� ���� ť
	std::atomic_int m_workerJobCnt;							/// ���� �۾� ���� - PPL�� cQ�� size�� ��Ȯ���� ����.
	std::condition_variable m_notifier;						/// �۾� �߻� �뺸�� Condition Variable
	std::vector<std::thread> m_workerThreads;				/// �۾� ������ ���� �����̳�

	concurrency::concurrent_queue<UserJob*> m_userQueue;	/// ���� �۾� ���� ť
	std::atomic_int m_userJobCnt;							/// ���� �۾� ���� - PPL�� cQ�� size�� ��Ȯ���� ����.

	///�۾��� ������ ����
	void InitMultiThread();
	///�۾��� �Լ�
	void WorkerThread();

	void PushUserJob(UserJob* jobPtr);

	public:
	///�۾� �߰� �� �뺸, ���� : user - ��� ������ ������, ev_type - �̺�Ʈ Ÿ��, data - ó���� ������
	void PushThreadJob(MainJob* jobPtr);
};

//@brief ChatServer ȣ�� ��ũ��.
#define g_chatServer (ChatServer::Instance())