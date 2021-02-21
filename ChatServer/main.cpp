#include "ChatServer.h"

int main()
{
	CHATSERVER chatServer;
	chatServer.Initialize(15600);
	chatServer.Run();
	chatServer.Terminate();
	return 0;
}