// Server

#include "pch.h"

#pragma comment(lib, "Ws2_32.lib")

#include "Shared.h"

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <map>

struct ConnectionInfo
{
	bool IsActive;
};

std::map<SOCKET, ConnectionInfo> gConnections;
int gConnectionCount = 0;
std::mutex gConnectionsMutex;
std::mutex gOutputMutex;

void CloseSocket(SOCKET socket)
{
	std::cerr << "Closing connection " << socket << std::endl;
	closesocket(socket);
	gConnections[socket].IsActive = false;
}

bool HandleMessage(SOCKET socket)
{
	bool result;

	std::string message;
	result = Net::ReceiveBuffer(socket, message);
	if (!result)
	{
		std::cerr << "Error reading message from client." << std::endl;
		CloseSocket(socket);
		return false;
	}

	{
		std::lock_guard<std::mutex> guard(gOutputMutex);
		std::cout << socket << ":" << message << std::endl;
	}

	{
		std::lock_guard<std::mutex> guard(gConnectionsMutex);
		for (const auto& pair : gConnections)
		{
			SOCKET targetSocket = pair.first;

			if (socket == targetSocket)
				continue;

			if (!pair.second.IsActive)
				continue;

			result = Net::SendMessagePacket(targetSocket, message);
			if (!result)
			{
				std::cerr << "Failed to send message to socket " << targetSocket << ".";
				CloseSocket(socket);
			}
		}
	}

	return true;
}

void ClientHandlingThread(SOCKET socket)
{
	bool result;

	const char motd[] = "Welcome to the server!";
	result = Net::SendMessagePacket(socket, motd, sizeof(motd));
	if (!result)
	{
		std::cerr << "Error sending welcome message to client.";
		CloseSocket(socket);
	}

	bool shouldContinue = true;
	do
	{
		Net::PacketType type;
		result = Net::ReceiveHeader(socket, type);

		if (result)
		{
			switch (type)
			{
				case Net::PacketType::Message:
				{
					result = HandleMessage(socket);

					if (!result)
					{
						shouldContinue = false;
					}
				}
				break;

				default:
				{
					std::cerr << "Unhandled packet type!" << (int)type << std::endl;
					shouldContinue = false;
				}
			}
		}
		else
		{
			std::cerr << "Error receieving header from client." << std::endl;
			shouldContinue = false;
		}
	} while (shouldContinue);

	CloseSocket(socket);
}

int main()
{
	Net::Init();

	SOCKADDR_IN addr;
	InetPtonW(AF_INET, L"127.0.0.1", &addr.sin_addr);
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	std::cout << "Listening on port " << addr.sin_port << std::endl;

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	bind(listenSocket, (SOCKADDR*)&addr, sizeof(addr));
	listen(listenSocket, SOMAXCONN);

	while (true)
	{
		int addrLen = sizeof(addr);

		SOCKET connectionSocket = accept(
			listenSocket,
			(SOCKADDR*)&addr,
			&addrLen);
		if (connectionSocket == 0)
		{
			std::cout << "Failed to accept the client's connection!" << std::endl;
		}
		else
		{
			int clientIndex = gConnectionCount++;

			{
				std::lock_guard<std::mutex> guard(gOutputMutex);
				std::cout << "Client " << clientIndex << " connected!" << std::endl;
			}

			{
				std::lock_guard<std::mutex> guard(gConnectionsMutex);
				gConnections.insert(std::pair<SOCKET, ConnectionInfo>(
					connectionSocket,
					{ true }));
			}

			std::thread t(ClientHandlingThread, connectionSocket);
			t.detach();
		}
	}

	closesocket(listenSocket);

	system("pause");

	return 0;
}
