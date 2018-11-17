#pragma once

#include <WinSock2.h>

#include <mutex>
#include <thread>

class Client
{
public:
	Client();
	~Client();

	bool Connect(const char* pIpAddress, int port);
	bool HandleInput(const std::string& input);

private:
	bool ReceiveMessage();
	void ServerRecieveThread();

	SOCKET gConnection;
	std::mutex gOutputMutex;
	std::thread m_recieveThread;
};

