// Server

#include "pch.h"

#pragma comment(lib, "Ws2_32.lib")

#include "Server.h"

#include "Shared.h"

#include <ws2tcpip.h>
#include <iostream>

Server::Server()
{}

Server::~Server()
{}

bool Server::Listen(int port, bool local)
{
	int result;
	AddressType addr;

	if (local)
	{
		InetPtonA(AF_INET, "127.0.0.1", &addr.sin_addr);
	}
	else
	{
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;

	m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	result = bind(m_listenSocket, (SOCKADDR*)&addr, sizeof(addr));
	if (result != 0)
	{
		std::cerr << "Failed to bind listening socket." << std::endl;
		return false;
	}

	result = listen(m_listenSocket, SOMAXCONN);
	if (result != 0)
	{
		std::cerr << "Failed to create listening socket." << std::endl;
		return false;
	}

	std::thread t(&Server::ListenThread, this);
	t.detach();

	std::cout << "Listening on port " << port << std::endl;

	return true;
}

void Server::CloseSocket(SOCKET socket)
{
	std::cerr << "Closing connection " << socket << std::endl;
	closesocket(socket);
	m_connections[socket].IsActive = false;
}

bool Server::HandleMessage(SOCKET socket)
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
		std::lock_guard<std::mutex> guard(m_outputMutex);
		std::cout << socket << ":" << message << std::endl;
	}

	{
		std::lock_guard<std::mutex> guard(m_connectionsMutex);
		for (const auto& pair : m_connections)
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

void Server::ClientHandlingThread(SOCKET socket)
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

void Server::ListenThread()
{
	while (true)
	{
		AddressType addr;
		int addrLen = sizeof(addr);

		SOCKET connectionSocket = accept(
			m_listenSocket,
			(SOCKADDR*)&addr,
			&addrLen);
		if (connectionSocket == 0)
		{
			std::cout << "Failed to accept the client's connection!" << std::endl;
		}
		else
		{
			int clientIndex = m_connectionCount++;

			{
				std::lock_guard<std::mutex> guard(m_outputMutex);
				std::cout << "Client " << clientIndex << " connected!" << std::endl;
			}

			{
				std::lock_guard<std::mutex> guard(m_connectionsMutex);
				m_connections.insert(std::pair<SOCKET, ConnectionInfo>(
					connectionSocket,
					{ true }));
			}

			std::thread t(&Server::ClientHandlingThread, this, connectionSocket);
			t.detach();
		}
	}
}

