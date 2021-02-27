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
	*@brief 서버를 초기화한다.
	*@param[in] 서버의 포트 번호.
	*@return 성공 시 true, 실패 시 false.
	*/
	bool Initialize(short port = SERVER_PORT);

	/**
	*@brief Accept를 시작한다.
	*/
	void Run();

	/**
	*@brief 서버의 종료처리를 한다.
	*/
	void Terminate();

private:
	SOCKET m_listener{INVALID_SOCKET};				///< Listen 소켓
	CmdParser m_cmdParser;							///< 명령어 처리객체

	ChatServer() {};

	/**
	*@brief WSA환경 초기화.
	*@param[in] 서버의 포트 번호.
	*@return 성공 시 true, 실패 시 false.
	*/
	bool InitWSA(short port);

	/**
	*@brief 로비를 생성한다.
	*@return 성공 시 true, 실패 시 false.
	*/
	bool InitLobby();

	/**
	*@brief 패킷을 처리한다.
	*@param[in] user 요청한 유저의 소켓.
	*@param[in] data 처리할 데이터.
	*/
	void ProcessPacket(const UserJob* jobPtr);

	// 명령어 처리 함수들.
	/**
	*@brief 로그인 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*@param[in] userName 로그인할 이름.
	*/
	void ProcessLogin(User* user, const std::string& userName);

	/**
	*@brief 채팅 패킷을 처리한다.
	*@param[in] user 송신자의 포인터.
	*@param[in] msg 보낼 메세지.
	*/
	void ProcessChat(User* sender, const std::string& msg);

	/**
	*@brief 입장 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*@param[in] userName 로그인할 방의 인덱스.
	*/
	void ProcessJoin(User* user, int roomIdx);

	/**
	*@brief 퇴장 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*/
	void ProcessQuit(User* user);

	/**
	*@brief 귓속말 패킷을 처리한다.
	*@param[in] sender 송신자의 포인터.
	*@param[in] receiverName 수신자의 이름.
	*@param[in] msg 보낼 메세지.
	*/
	void ProcessMsg(User* sender, const std::string& receiverName, const std::string& msg);

	/**
	*@brief 현재 방 내 유저 목록 요청 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*/
	void ProcessGetUserList(User* user);

	/**
	*@brief 방 목록 요청 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*/
	void ProcessGetRoomList(User* user);

	/**
	*@brief 서버 내 유저 목록 요청 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*/
	void ProcessGetAllUserList(User* user);

	/**
	*@brief 방 생성 요청 패킷을 처리한다. 유저는 방 생성 후 바로 해당 방으로 입장한다.
	*@param[in] user 요청한 유저의 포인터.
	*@param[in] roomName 생성할 방의 이름.
	*@param[in] maxUser 방의 인원 제한 수.
	*/
	void ProcessCreateRoom(User* user, const std::string& roomName, int maxUser);

	/**
	*@brief 도움말 요청 패킷을 처리한다.
	*@param[in] user 요청한 유저의 포인터.
	*/
	void ProcessHelp(User* user);

	/**
	*@brief 잘못된 명령어를 수신받았을 때의 처리함수. 유저에게 오류 메세지를 보낸다.
	*@param[in] user 요청한 유저의 포인터.
	*/
	void ProcessError(User* user);


	///select 기반으로 IOCP와 비슷한 모델을 구축하는 것을 목표로 한다.
	///APC Queue로 PPL의 concurrent_queue를 사용.
	///GSCQ를 c++의 condition_variable로 대체한다.
	///MultiThread.
	concurrency::concurrent_queue<MainJob*> m_workerQueue;	/// 작업자 작업 저장 큐
	std::atomic_int m_workerJobCnt;							/// 남은 작업 갯수 - PPL의 cQ의 size는 정확하지 않음.
	std::condition_variable m_notifier;						/// 작업 발생 통보용 Condition Variable
	std::vector<std::thread> m_workerThreads;				/// 작업 스레드 보관 컨테이너

	concurrency::concurrent_queue<UserJob*> m_userQueue;	/// 유저 작업 저장 큐
	std::atomic_int m_userJobCnt;							/// 남은 작업 갯수 - PPL의 cQ의 size는 정확하지 않음.

	///작업자 스레드 생성
	void InitMultiThread();
	///작업자 함수
	void WorkerThread();

	void PushUserJob(UserJob* jobPtr);

	public:
	///작업 추가 및 통보, 인자 : user - 대상 유저의 포인터, ev_type - 이벤트 타입, data - 처리할 데이터
	void PushThreadJob(MainJob* jobPtr);
};

//@brief ChatServer 호출 매크로.
#define g_chatServer (ChatServer::Instance())