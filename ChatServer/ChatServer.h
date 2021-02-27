#pragma once
#pragma comment(lib, "ws2_32")

///NW Headers
#include <ws2tcpip.h>
#include <fcntl.h>

#include <iostream>
#include <string>
#include <map>
#include <assert.h>
#include <algorithm>

#include "Room.h"
#include "User.h"
#include "CmdParser.h"
#include "Config.h"
#include "Logger.h"
#include "UtilFunc.h"
#include "Session.h"


///MultiThread
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <concurrent_queue.h>
#include "Job.h"

class ChatServer
{
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
	std::vector<std::thread> m_recvThreads;

	concurrency::concurrent_queue<UserJob*> m_userQueue;	/// 유저 작업 저장 큐
	std::atomic_int m_userJobCnt;							/// 남은 작업 갯수 - PPL의 cQ의 size는 정확하지 않음.

	concurrency::concurrent_queue<SessionTable*>	m_sessionQueue; /// RecvThread 중 소켓 갯수가 일정 수 이하로 감소한 스레드들의 소켓.

	/**
	*@brief 작업자 스레드를 생성한다.
	*/
	void InitMultiThread();

	/**
	*@brief send와 명령어 파싱 처리를 한다.
	*/
	void WorkerThread();

	/**
	*@brief select Folk thread Function. SOCKET_LOWER_BOUND 숫자 이하로 세션이 감소하면 메인스레드와 결합한다.
	*@param[in] sessions folk할 세션들이 담긴 SessionTable 포인터.
	*/
	void RecvThread(SessionTable* sessions);

	/**
	*@brief 메인 작업을 cQ에 삽입한다. 가장 먼저 삽입한 스레드가 선점권을 갖고 큐를 수행한다.
	*@param[in] jobPtr 추가할 작업의 포인터.
	*/
	void PushUserJob(UserJob* jobPtr);

	public:
	/**
	*@brief WorkerThread에 작업을 통보한다. 작업의 종류는 send와 명령어 파싱, 접속/종료 통보가 있다.
	*@paran[in] jobPtr 추가할 작업의 포인터.
	*/
	void PushThreadJob(MainJob* jobPtr);
};

//@brief ChatServer 호출 매크로.
#define g_chatServer (ChatServer::Instance())