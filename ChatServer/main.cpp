#include <iostream>

#include "ChatServer.h"
#include "UtilFunc.h"

int main(int argc, char** argv)
{
	std::string errMsg{ "ChatServer [0 - 65535]\n��Ʈ��ȣ�� ��Ȯ�� �Է����ּ���." };

	//�߰� ���ڰ� �ԷµǸ� ��Ʈ�� �����Ѵ�.
	short customPort = 0;
	bool customPortFlag = false;
	if (argc > 1)
	{
		customPortFlag = PortParse(argv[1], customPort);
		//��Ʈ ���� ���� �� �����޼��� ��� �� ����.
		if (false == customPortFlag)
		{
			std::cout << errMsg << std::endl;
			return -1;
		}
	}
	
	setlocale(LC_ALL, "KOREAN");
	bool isOk = false;
	//�Էµ� ��Ʈ�� ������ �ش� ��Ʈ�� �ʱ�ȭ.
	if (true == customPortFlag)
	{
		isOk = g_chatServer.Initialize(customPort);
	}
	else
	//������ �⺻ ��Ʈ�� �ʱ�ȭ.
	{
		isOk = g_chatServer.Initialize();
	}

	if (true == isOk)
	{
		g_chatServer.Run();
		g_chatServer.Terminate();
		return 0;
	}
	else
	{
		std::cout << "���� �ʱ�ȭ ����" << std::endl;
		return -1;
	}
}