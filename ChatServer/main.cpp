#include <iostream>

#include "ChatServer.h"
#include "UtilFunc.h"

int main(int argc, char** argv)
{
	std::string errMsg{ "ChatServer [0 - 65535]\n포트번호를 정확히 입력해주세요." };

	//추가 인자가 입력되면 포트를 추출한다.
	short customPort = 0;
	bool customPortFlag = false;
	if (argc > 1)
	{
		customPortFlag = PortParse(argv[1], customPort);
		//포트 추출 실패 시 오류메세지 출력 후 종료.
		if (false == customPortFlag)
		{
			std::cout << errMsg << std::endl;
			return -1;
		}
	}
	
	setlocale(LC_ALL, "KOREAN");
	ChatServer chatServer;
	//입력된 포트가 있으면 해당 포트로 초기화.
	if (true == customPortFlag)
	{
		chatServer.Initialize(customPort);
	}
	else
	//없으면 기본 포트로 초기화.
	{
		chatServer.Initialize();
	}

	chatServer.Run();
	chatServer.Terminate();
	return 0;
}