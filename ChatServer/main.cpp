#include "ChatServer.h"

int main()
{
	ChatServer chatServer;
	chatServer.Initialize(15600);
	chatServer.Run();
	chatServer.Terminate();
	return 0;
}