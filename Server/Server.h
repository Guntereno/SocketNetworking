#pragma once

#include <map>
#include <mutex>
#include <thread>

#include <WinSock2.h>

#include "Shared.h"


class Server
{
public:
	Server();
	~Server();

	bool Listen(int port, bool local);
	bool CheckForReadyState();

private:
	typedef SOCKADDR_IN AddressType;

	struct ConnectionInfo
	{
		bool IsConnected;
		bool IsReady;
		u32 IpAddress;
		u16 UdpPort;
	};

	void CloseSocket(SOCKET socket);
	bool HandleMessage(SOCKET socket);
	void ClientHandlingThread(SOCKET socket);
	void ListenThread();
	int GetFirstFreeSlot();

	SOCKET m_slots[Net::kNumSlots];
	std::map<SOCKET, ConnectionInfo> m_connections;

	int m_connectionCount = 0;
	std::recursive_mutex m_connectionsMutex;
	SOCKET m_listenSocket = INVALID_SOCKET;

};

