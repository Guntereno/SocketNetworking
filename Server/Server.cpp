// Server

#include "pch.h"

#pragma comment(lib, "Ws2_32.lib")

#include "Shared.h"

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <mutex>

static const size_t kMaxConnections = 128;
SOCKET gConnections[kMaxConnections];
int gConnectionCount = 0;
std::mutex gConnectionsMutex;
std::mutex gOutputMutex;

void ClientHandlingThread(int index)
{
	SOCKET socket = gConnections[index];

	const char motd[] = "Welcome to the server!";
	Net::SendBuffer(socket, motd, sizeof(motd));

	bool shouldContinue = true;
	do
	{
		std::string message = Net::ReceiveBuffer(socket);

		{
			std::lock_guard<std::mutex> guard(gOutputMutex);
			std::cout << index << ":" << message << std::endl;
		}

		{
			std::lock_guard<std::mutex> guard(gConnectionsMutex);
			for (int i = 0; i < gConnectionCount; ++i)
			{
				if (i == index)
					continue;

				Net::SendBuffer(gConnections[i], message);
			}
		}
	} while (shouldContinue);
}

int main()
{
	Net::Init();

	SOCKADDR_IN addr;
	InetPtonW(AF_INET, L"127.0.0.1", &addr.sin_addr);
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	bind(listenSocket, (SOCKADDR*)&addr, sizeof(addr));
	listen(listenSocket, SOMAXCONN);

	while (gConnectionCount < kMaxConnections)
	{
		int addrLen = sizeof(addr);

		SOCKET connectionSocket = accept(
			listenSocket,
			(SOCKADDR*)&addr,
			&addrLen);
		if (connectionSocket == 0)
		{
			std::cout << "Failed to accept the client's connection!\n";
		}
		else
		{
			int clientIndex = gConnectionCount;

			{
				std::lock_guard<std::mutex> guard(gOutputMutex);
				std::cout << "Client " << clientIndex << " connected!\n";
			}

			{
				std::lock_guard<std::mutex> guard(gConnectionsMutex);
				gConnections[gConnectionCount++] = connectionSocket;
			}

			std::thread t(ClientHandlingThread, clientIndex);
			t.detach();
		}
	}

	system("pause");

	return 0;
}
