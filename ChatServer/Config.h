#pragma once
constexpr short SERVER_PORT = 15600;		///< ���� ��Ʈ.
constexpr int BUF_SIZE = 1024;				///< RECVThread�� ���� ũ��.
constexpr int USERBUF_SIZE = 2048;			///< ���� ���� �ִ��� ������ �� �ִ� ������ ������ ����.
constexpr int MAX_LOBBY_SIZE = 10000;		///< �κ��� �������̺� ���� ũ��.
constexpr int MAX_RESERVE_USERSIZE = 100;	///< �� ���� �� ������ ������ �ִ� ũ��(vector.reserve())
constexpr int WORKERTHREAD_NUM = 8;			///< �۾��� �������� ����
constexpr int SELECT_TIMEOUT = 500;			///< ���� listen������ Ÿ�Ӿƿ� ����(ms).
constexpr int SOCKET_LOWER_BOUND = 20;		///< �ش� ���� ������ �� ���ϸ� recvThread�� �۵��� �����ϰ� ���ν������ ������.

constexpr int LOBBY_INDEX = 0;
constexpr int OUT_OF_RANGE = -1;
constexpr int MINUSER_NUM = 2;				///< ���� �ּ� ���� �ο�.
constexpr int MAXUSER_NUM = 10;				///< ���� �ִ� ���� �ο�.
constexpr int MAX_IDLENGTH = 10;			///< �ִ� ���̵� ����.

constexpr char LOG_PATH[] =  R"(.\Log.txt)" ;		///< �α����� ���� ���.
constexpr char WSALOG_PATH[] = R"(.\WSALog.txt)";	///< WSA�α����� ���� ���.

