#pragma once

#include <map>
#include <mutex>
#include <thread>

#include <WinSock2.h>

class Server
{
public:
	Server();
	~Server();

	bool Listen(int port, bool local);

private:
	typedef SOCKADDR_IN AddressType;

	struct ConnectionInfo
	{
		bool IsActive;
	};

	void CloseSocket(SOCKET socket);
	bool HandleMessage(SOCKET socket);
	void ClientHandlingThread(SOCKET socket);
	void ListenThread();

	std::map<SOCKET, ConnectionInfo> m_connections;
	int m_connectionCount = 0;
	std::mutex m_connectionsMutex;
	std::mutex m_outputMutex;
	SOCKET m_listenSocket;
};

