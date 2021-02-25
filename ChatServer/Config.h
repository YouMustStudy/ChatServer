#pragma once
constexpr short SERVER_PORT = 15600; ///< 서버 포트.
constexpr int BUF_SIZE = 1024;		 ///< RECVThread용 버퍼 크기.
constexpr int USERBUF_SIZE = 2048;	 ///< 엔터 없이 최대한 저장할 수 있는 유저의 데이터 길이.
constexpr int MINUSER_NUM = 2;		 ///< 방의 최소 입장 인원.
constexpr int MAXUSER_NUM = 100;	 ///< 방의 최대 입장 인원.
constexpr int MAX_IDLENGTH = 10;

constexpr char LOG_PATH[] =  R"(.\Log.txt)" ;		///< 로그파일 저장 경로.
constexpr char WSALOG_PATH[] = R"(.\WSALog.txt)";	///< WSA로그파일 저장 경로.