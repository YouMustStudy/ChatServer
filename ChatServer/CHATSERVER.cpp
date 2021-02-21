#include "CHATSERVER.h"


bool CHATSERVER::Initialize(short port)
{
    InitWSA(port);
    return true;
}

void CHATSERVER::Run()
{
    char welcomeMsg[]("æ»≥Á«œººø‰\n");
    std::cout << "[Start Accept]" << std::endl;

    fd_set masterFds, copyFds;
    FD_ZERO(&masterFds);
    FD_SET(m_listener, &masterFds);
    SOCKET maxFd = m_listener;

    char buffer[BUF_SIZE + 1];
    std::string data;

    while (true)
    {
        copyFds = masterFds;
        int numFd = select(static_cast<int>(maxFd) + 1, &copyFds, 0, 0, 0);
        assert(numFd > 0);

        for (int readySoc = 0; readySoc < maxFd + 1; ++readySoc)
        {
            if (FD_ISSET(readySoc, &copyFds))
            {
                /// Accept
                if (m_listener == readySoc)
                {
                    int len = sizeof(SOCKADDR_IN);
                    SOCKADDR_IN clientAddr;
                    SOCKET clientSocket = accept(m_listener, reinterpret_cast<sockaddr*>(&clientAddr), &len);

                    //Non-Blocking Socket
                    //u_long nonBlockingMode = 1;
                    //ioctlsocket(clientSocket, FIONBIO, &nonBlockingMode);

                    FD_SET(clientSocket, &masterFds);
                    maxFd = max(maxFd, clientSocket);

                    std::cout << "Client Accept" << std::endl;
                    send(clientSocket, welcomeMsg, int(strlen(welcomeMsg)) + 1, 0);
                }
                /// Recv
                else
                {
					int recvLength = recv(readySoc, reinterpret_cast<char*>(buffer), BUF_SIZE, 0);
                    if (recvLength == 0)
                    {
                        FD_CLR(readySoc, &masterFds);
                        closesocket(readySoc);
                        continue;
                    }

					buffer[recvLength] = '\0';
					data.append(buffer);

					while (true)
					{
						size_t cmdPos = data.find("\r\n");
						if (std::string::npos != cmdPos)
						{
							ProcessPacket(readySoc, data.substr(0, cmdPos+2));
							data = data.substr(cmdPos + 2);
						}
						else break;
					}
                }
            }
        }
    }
}

void CHATSERVER::Terminate()
{
    closesocket(m_listener);
    WSACleanup();
}

bool CHATSERVER::InitWSA(short port)
{
    std::cout << "[Initializing ChatServer" << " - " << port << "]" << std::endl;
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cout << "Error - WSAStartup()" << std::endl;
        return false;
    }

    SOCKADDR_IN addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    m_listener = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(m_listener, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
    {
        std::cout << "Error - Bind()" << std::endl;
        return false;
    }

    if (listen(m_listener, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cout << "Error - Listen()" << std::endl;
        return false;
    }

    return true;
}

void CHATSERVER::ProcessPacket(int client, std::string data)
{
    send(client, data.c_str(), static_cast<int>(data.size()), 0);
}
