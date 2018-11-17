// Server

#include "pch.h"

#pragma comment(lib, "Ws2_32.lib")

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

void InitWinsock()
{
	WSAData wsaData;
	WORD d11Version = MAKEWORD(2, 1);
	if (WSAStartup(d11Version, &wsaData) != 0)
	{
		MessageBoxA(nullptr, "Failed to start Winsock", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}
}

void ClientHandlingThread(int index)
{
	SOCKET socket = gConnections[index];

	char buffer[256] = "Welcome to the server!";
	send(socket, buffer, sizeof(buffer), 0);

	bool shouldContinue = true;
	do
	{
		recv(socket, buffer, sizeof(buffer), 0);

		{
			std::lock_guard<std::mutex> guard(gOutputMutex);
			std::cout << index << ":" << buffer << std::endl;
		}

		{
			std::lock_guard<std::mutex> guard(gConnectionsMutex);
			for (int i = 0; i < gConnectionCount; ++i)
			{
				if (i == index)
					continue;

				send(gConnections[i], buffer, sizeof(buffer), 0);
			}
		}
	} while (shouldContinue);
}

int main()
{
	InitWinsock();

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
