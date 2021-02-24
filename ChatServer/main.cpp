#include "ChatServer.h"

int main()
{
	setlocale(LC_ALL, "KOREAN");
	ChatServer chatServer;
	chatServer.Initialize(15600);
	chatServer.Run();
	chatServer.Terminate();
	return 0;
}