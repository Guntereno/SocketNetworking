// Client

#include "pch.h"

#include "Client.h"

#pragma comment(lib, "Ws2_32.lib")

#include "Shared.h"

#include <ws2tcpip.h>
#include <iostream>
#include <memory>
#include <string>

Client::Client():
	m_connection(-1)
{}

Client::~Client()
{
	closesocket(m_connection);
}

bool Client::Connect(const char* pIpAddress, int port)
{
	SOCKADDR_IN addr;

	int result = InetPtonA(AF_INET, pIpAddress, &addr.sin_addr);
	if (result != 1)
	{
		std::cerr << "Invalid IP address!" << std::endl;
		return false;
	}

	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;

	int addrLen = sizeof(addr);
	m_connection = socket(AF_INET, SOCK_STREAM, 0);

	result = connect(m_connection, (SOCKADDR*)(&addr), addrLen);
	if (result != 0)
	{
		std::cout << "Failed to connect to server!" << std::endl;
		return false;
	}

	std::thread t(&Client::ServerRecieveThread, this);
	t.detach();

	return true;
}

bool Client::HandleInput(const std::string& input)
{
	return Net::SendMessagePacket(m_connection, input);
}

bool Client::ReceiveMessage()
{
	bool result;

	std::string message;
	result = Net::ReceiveBuffer(m_connection, message);

	if (result)
	{
		std::lock_guard<std::mutex> guard(m_outputMutex);
		std::cout << message << std::endl;
		return true;
	}

	return false;
}

void Client::ServerRecieveThread()
{
	bool shouldContinue = true;
	do
	{
		Net::PacketType type;
		shouldContinue = Net::ReceiveHeader(m_connection, type);

		if (shouldContinue)
		{
			switch (type)
			{
				case Net::PacketType::Message:
				{
					shouldContinue = ReceiveMessage();
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
	closesocket(m_connection);
}
