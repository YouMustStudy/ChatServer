#include "ChatServer.h"


bool ChatServer::Initialize(short port)
{
    InitWSA(port);



	m_lobby = m_roomMgr.CreateRoom("Lobby");
	if (nullptr == m_lobby)
		return false;

    return true;
}

void ChatServer::Run()
{
    char welcomeMsg[]("=====================\r\nWelcome To ChatServer\r\n=====================\r\n");
    std::cout << "[Start Accept]" << std::endl;

    fd_set masterFds, copyFds;
    FD_ZERO(&masterFds);
    FD_SET(m_listener, &masterFds);
    SOCKET maxFd = m_listener;

    char buffer[BUF_SIZE + 1];
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
                    FD_SET(clientSocket, &masterFds);
                    maxFd = max(maxFd, clientSocket);

                    std::cout << "Client Accept - " << clientSocket << std::endl;
                    send(clientSocket, welcomeMsg, int(strlen(welcomeMsg)) + 1, 0);

                    //Non-Blocking Socket
                    //u_long nonBlockingMode = 1;
                    //ioctlsocket(clientSocket, FIONBIO, &nonBlockingMode);

                    m_userTable.emplace(std::make_pair(clientSocket, new User(clientSocket)));
                    m_userTable[clientSocket]->id = std::to_string(clientSocket);
                    if (true == m_lobby->Enter(m_userTable[clientSocket]))
                    {
                        m_userTable[clientSocket]->m_Room = m_lobby;
                    }
                }
                /// Recv
                else
                {
					int recvLength = recv(readySoc, reinterpret_cast<char*>(buffer), BUF_SIZE, 0);
                    UserPtr user = m_userTable[readySoc];
                    if (recvLength == 0)
                    {
						FD_CLR(readySoc, &masterFds);
						DisconnectUser(user);
                        continue;
                    }

					buffer[recvLength] = '\0';
                    std::string& data = user->data;
                    data.append(buffer);

					while (true)
					{
						size_t cmdPos = data.find("\r\n");
						if (std::string::npos != cmdPos)
						{
							ProcessPacket(user, data.substr(0, cmdPos+2));
							data = data.substr(cmdPos + 2);
						}
						else break;
					}
                }
            }
        }
    }
}

void ChatServer::Terminate()
{
    closesocket(m_listener);
    WSACleanup();
}

bool ChatServer::InitWSA(short port)
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

void ChatServer::ProcessPacket(UserPtr& user, std::string data)
{
    if (nullptr != user->m_Room)
    {
        user->m_Room->SendChat(user, data);
    }
}

void ChatServer::DisconnectUser(UserPtr& user)
{
	if (true == user->m_Room->Leave(user))
	{
		user->m_Room = nullptr;
	}
	closesocket(user->m_socket);
	std::cout << "Client Out - " << user->id << std::endl;
	m_userTable.erase(user->m_socket);
}