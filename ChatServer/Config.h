#pragma once
constexpr short SERVER_PORT = 15600;		///< 서버 포트.
constexpr int BUF_SIZE = 1024;				///< RECVThread용 버퍼 크기.
constexpr int USERBUF_SIZE = 2048;			///< 엔터 없이 최대한 저장할 수 있는 유저의 데이터 길이.
constexpr int MAX_LOBBY_SIZE = 10000;		///< 로비의 유저테이블 시작 크기.
constexpr int MAX_RESERVE_USERSIZE = 100;	///< 방 생성 시 예약할 유저의 최대 크기(vector.reserve())
constexpr int WORKERTHREAD_NUM = 8;			///< 작업자 스레드의 갯수
constexpr int SELECT_TIMEOUT = 500;			///< 메인 listen스레드 타임아웃 간격(ms).
constexpr int SOCKET_LOWER_BOUND = 20;		///< 해당 소켓 갯수가 값 이하면 recvThread가 작동을 중지하고 메인스레드와 합쳐짐.

constexpr int LOBBY_INDEX = 0;
constexpr int OUT_OF_RANGE = -1;
constexpr int MINUSER_NUM = 2;				///< 방의 최소 입장 인원.
constexpr int MAXUSER_NUM = 10;				///< 방의 최대 입장 인원.
constexpr int MAX_IDLENGTH = 10;			///< 최대 아이디 길이.

constexpr char LOG_PATH[] =  R"(.\Log.txt)" ;		///< 로그파일 저장 경로.
constexpr char WSALOG_PATH[] = R"(.\WSALog.txt)";	///< WSA로그파일 저장 경로.

