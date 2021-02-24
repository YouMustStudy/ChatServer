#include "Error.h"

std::string get_time()
{
	time_t current_time = time(nullptr);
	struct tm current_tm;
	localtime_s(&current_tm, &current_time);

	int year = current_tm.tm_year + 1900;
	int month = current_tm.tm_mon + 1;
	int day = current_tm.tm_mday;
	int hour = current_tm.tm_hour;
	int minute = current_tm.tm_min;
	int second = current_tm.tm_sec;

	char buf[34];
	sprintf_s(buf, "[%4d-%02d-%02d %02d:%02d:%02d], ", year, month, day, hour, minute, second);
	return buf;
}

void error_display(const char * msg, int errNo)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errNo,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)& lpMsgBuf, 0, NULL);

	std::string errorTime = get_time();

	std::cout << errorTime << msg;
	std::wcout << L" [Error]" << lpMsgBuf << std::endl;
	LocalFree(lpMsgBuf);

	std::ofstream logFile("ErrorLog.txt", std::ios::app);
	logFile << errorTime << msg << L" [Error]" << lpMsgBuf << std::endl;
}
