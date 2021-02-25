#pragma once
constexpr short SERVER_PORT = 15600; ///< ���� ��Ʈ.
constexpr int BUF_SIZE = 1024;		 ///< RECVThread�� ���� ũ��.
constexpr int USERBUF_SIZE = 2048;	 ///< ���� ���� �ִ��� ������ �� �ִ� ������ ������ ����.
constexpr int MINUSER_NUM = 2;		 ///< ���� �ּ� ���� �ο�.
constexpr int MAXUSER_NUM = 100;	 ///< ���� �ִ� ���� �ο�.
constexpr int MAX_IDLENGTH = 10;

constexpr char LOG_PATH[] =  R"(.\Log.txt)" ;		///< �α����� ���� ���.
constexpr char WSALOG_PATH[] = R"(.\WSALog.txt)";	///< WSA�α����� ���� ���.