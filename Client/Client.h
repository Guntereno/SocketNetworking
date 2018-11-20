#pragma once

#include <WinSock2.h>

#include <mutex>
#include <thread> 

#include "Types.h"

class Client
{
public:
	Client();
	~Client();

	bool ConnectToLobby(const char* pIpAddress, int port);
	bool HandleInput(const std::string& input);

private:
	bool ReceiveMessage();
	void ServerRecieveThread();
	bool OpenGameplaySocket(u16& o_port);

	SOCKET m_lobbySocket;
	SOCKET m_gameSocket;
	std::thread m_receiveThread;
};

