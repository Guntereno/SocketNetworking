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

bool RecieveMessage()
{
	bool result;

	std::string message;
	result = Net::ReceiveBuffer(gConnection, message);

	if(result)
	{
		std::lock_guard<std::mutex> guard(gOutputMutex);
		std::cout << message << std::endl;
		return true;
	}

	return false;
}

void ServerRecieveThread()
{
	bool result;

	bool shouldContinue = true;
	do
	{
		Net::PacketType type;
		shouldContinue = Net::ReceiveHeader(gConnection, type);

		if (shouldContinue)
		{
			switch (type)
			{
				case Net::PacketType::Message:
				{
					shouldContinue = RecieveMessage();
				}
				break;

				default:
				{
					std::cerr << "Unhandled packet type!: " << (int)type << std::endl;
					shouldContinue = false;
				}
			}
		}
	} while (shouldContinue);

	std::cerr << "Lost connection to server." << std::endl;
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
		std::cout << "Failed to connect to server!" << std::endl;
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

			shouldContinue = Net::SendMessagePacket(gConnection, buffer);
		} while (shouldContinue);

		std::cerr << "Lost connection to server." << std::endl;
		closesocket(gConnection);
	}

	system("pause");
}
