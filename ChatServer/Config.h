#pragma once
constexpr short SERVER_PORT = 15600; ///< 서버 포트.
constexpr int BUF_SIZE = 1024;		 ///< RECV용 버퍼 크기.
constexpr int MINUSER_NUM = 2;		 ///< 방의 최소 입장 인원.

constexpr char LOG_PATH[] =  R"(.\Log.txt)" ;		///< 로그파일 저장 경로.
constexpr char WSALOG_PATH[] = R"(.\WSALog.txt)";	///< WSA로그파일 저장 경로.