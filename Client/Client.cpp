// Client

#include "pch.h"

#pragma comment(lib, "Ws2_32.lib")

#include "Shared.h"

#include <WinSock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

static SOCKET gConnection;
static std::mutex gOutputMutex;

void RecieveMessage()
{
	std::string message = Net::ReceiveBuffer(gConnection);

	{
		std::lock_guard<std::mutex> guard(gOutputMutex);
		std::cout << message << std::endl;
	}
}

void ServerRecieveThread()
{
	bool shouldContinue = true;
	do
	{
		Net::PacketType type = Net::ReceiveHeader(gConnection);
		switch (type)
		{
			case Net::PacketType::Message:
			{
				RecieveMessage();
			}
			break;

			default:
			{
				std::cerr << "Unhandled packet type!: " << (int)type << std::endl;
				shouldContinue = false;
			}
		}
	} while (shouldContinue);

	closesocket(gConnection);
}

int main()
{
	Net::Init();
 
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
		std::thread t(ServerRecieveThread);
		t.detach();

		std::string buffer;
		bool shouldContinue = true;
		do
		{
			std::getline(std::cin, buffer);
			Net::SendMessagePacket(gConnection, buffer);
		} while (shouldContinue);

		std::cout << "Connected!\n";
	}

	system("pause");
}
