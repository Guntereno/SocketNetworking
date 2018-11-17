// Client

#include "pch.h"

#pragma comment(lib, "Ws2_32.lib")

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <mutex>

static const size_t kBufferLen = 256;
static SOCKET gConnection;
static std::mutex gOutputMutex;

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

void ServerRecieveThread()
{
	char buffer[kBufferLen];

	bool shouldContinue = true;
	do
	{	
		recv(gConnection, buffer, sizeof(buffer), 0);
		
		{
			//std::lock_guard<std::mutex> guard(gOutputMutex);
			std::cout << buffer << std::endl;
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

	int addrLen = sizeof(addr);
	gConnection = socket(AF_INET, SOCK_STREAM, 0);
	int result = connect(gConnection, (SOCKADDR*)(&addr), addrLen);
	if(result != 0)
	{
		std::cout << "Failed to connect to server!\n";
	}
	else
	{
		bool shouldContinue = true;

		char buffer[kBufferLen];

		do
		{
			std::thread t(ServerRecieveThread);
			t.detach();

			std::cin.getline(buffer, sizeof(buffer));
			send(gConnection, buffer, sizeof(buffer), 0);
		} while (shouldContinue);

		std::cout << "Connected!\n";
	}

	system("pause");
}
