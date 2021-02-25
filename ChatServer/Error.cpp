#include "Error.h"
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
	
	//로그를 파일에 저장
	std::ofstream logFile("ErrorLog.txt", std::ios::app);
	logFile << errorTime << msg << L" [Error]" << lpMsgBuf << std::endl;

	LocalFree(lpMsgBuf);
}
