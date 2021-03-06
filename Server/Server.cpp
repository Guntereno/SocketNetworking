// Server

#include "pch.h"

#pragma comment(lib, "Ws2_32.lib")

#include "Server.h"

#include <ws2tcpip.h>
#include <iostream>


Server::Server()
{
	for (int i = 0; i < Net::kNumSlots; ++i)
	{
		m_slots[i] = INVALID_SOCKET;
	}
}

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

bool Server::CheckForReadyState()
{
	std::lock_guard<std::recursive_mutex> guard(m_connectionsMutex);

	bool allUsersReady = true;
	int numUsers = 0;

	for (int i = 0; i < Net::kNumSlots; ++i)
	{
		SOCKET slotSocket = m_slots[i];
		if (slotSocket != INVALID_SOCKET)
		{
			numUsers += 1;
			allUsersReady &= m_connections[slotSocket].IsReady;
		}
	}

	bool shouldStartGame = ((numUsers > 1) && allUsersReady);
	if (shouldStartGame)
	{
		for (int clientIndex = 0; clientIndex < Net::kNumSlots; ++clientIndex)
		{
			SOCKET slotSocket = m_slots[clientIndex];

			if (slotSocket == INVALID_SOCKET)
			{
				std::cout << +clientIndex << " is NULL" << std::endl;;
			}
			else
			{
				const ConnectionInfo& connectionInfo = m_connections[slotSocket];
				std::cout << clientIndex << " is ";
				Net::OutputIp(std::cout, connectionInfo.IpAddress);
				std::cout << ":" << connectionInfo.UdpPort << std::endl;
			}


			if (slotSocket != INVALID_SOCKET)
			{
				Net::SendHeader(slotSocket, Net::PacketType::StartGame);

				for (int partnerIndex = 0; partnerIndex < Net::kNumSlots; ++partnerIndex)
				{
					SOCKET partnerSocket = m_slots[partnerIndex];
					if (partnerIndex == clientIndex)
					{
						Net::SendHeader(slotSocket, Net::PacketType::ClientRecipient);
					}
					else if (partnerSocket == INVALID_SOCKET)
					{
						Net::SendHeader(slotSocket, Net::PacketType::ClientNull);
					}
					else
					{
						Net::SendHeader(slotSocket, Net::PacketType::ClientValue);
						const ConnectionInfo& connectionInfo = m_connections[partnerSocket];
						Net::SendU32(slotSocket, connectionInfo.IpAddress);
						Net::SendU16(slotSocket, connectionInfo.UdpPort);
					}
				}
			}
		}

		for (int i = 0; i < Net::kNumSlots; ++i)
		{
			if (m_slots[i] != INVALID_SOCKET)
			{
				CloseSocket(m_slots[i]);
			}
		}

		return true;
	}

	return false;
}

void Server::CloseSocket(SOCKET socket)
{
	std::lock_guard<std::recursive_mutex> guard(m_connectionsMutex);

	std::cerr << "Closing connection " << socket << std::endl;
	closesocket(socket);
	m_connections[socket].IsConnected = false;

	for (int i = 0; i < Net::kNumSlots; ++i)
	{
		if (m_slots[i] == socket)
		{
			m_slots[i] = INVALID_SOCKET;
		}
	}
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

	std::cout << socket << ":" << message << std::endl;

	{
		std::lock_guard<std::recursive_mutex> guard(m_connectionsMutex);
		for (const auto& pair : m_connections)
		{
			SOCKET targetSocket = pair.first;

			if (socket == targetSocket)
				continue;

			if (!pair.second.IsConnected)
				continue;

			result = Net::SendMessagePacket(targetSocket, message);
			if (!result)
			{
				std::cerr << "Failed to send message to socket " << targetSocket << "." << std::endl;;
				CloseSocket(socket);
			}
		}
	}

	return true;
}

int Server::GetFirstFreeSlot()
{
	for (int i = 0; i < Net::kNumSlots; ++i)
	{
		if (m_slots[i] == INVALID_SOCKET)
		{
			return i;
		}
	}

	// No free slots
	return -1;
}

void Server::ClientHandlingThread(SOCKET socket)
{
	bool result;

	bool shouldContinue = false;

	const char motd[] = "Welcome to the server!";
	result = Net::SendMessagePacket(socket, motd, sizeof(motd));
	if (result)
	{
		result = Net::SendHeader(socket, Net::PacketType::RequestOpenUdp);
		if (result)
		{
			Net::PacketType recievedHeader;
			result = Net::ReceiveHeader(socket, recievedHeader);
			if (result)
			{
				if (recievedHeader == Net::PacketType::AcknowledgeOpenUdp)
				{
					u16 port;
					result = Net::ReceiveU16(socket, port);
					if (result)
					{
						m_connections[socket].UdpPort = port;

						std::cout << "Client " << socket << " opened their UDP socket on port " << port << "." << std::endl;
						shouldContinue = true;
					}
					else
					{
						std::cerr << "Failed to receive UDP port from client." << std::endl;
					}
				}
				else
				{
					std::cerr << "Error receiving UDP acknowledgment." << std::endl;;
				}
			}
			else
			{
				std::cout << "Failed to receive client UDP acknowledgment header." << std::endl;
			}
		}
		else
		{
			std::cerr << "Error sending UDP request." << std::endl;;
		}
	}
	else
	{
		std::cerr << "Error sending welcome message to client." << std::endl;;
	}

	while (shouldContinue)
	{
		Net::PacketType type;
		result = Net::ReceiveHeader(socket, type);

		if (result)
		{
			switch (type)
			{
				case Net::PacketType::Message:
				{
					shouldContinue = HandleMessage(socket);
				}
				break;

				case Net::PacketType::Ready:
				{
					std::lock_guard<std::recursive_mutex> guard(m_connectionsMutex);

					m_connections[socket].IsReady = true;
					std::cout << "Player at socket " << socket << " is ready." << std::endl;
					shouldContinue = false;

					// We end the thread execution, but leave the socket open.
					// This is so we can tell the client to start the game
					return;
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
			std::cerr << "Error receiving header from client." << std::endl;
			shouldContinue = false;
		}
	}

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
			int clientIndex = GetFirstFreeSlot();
			if (clientIndex != -1)
			{
				std::cout << "Client " << clientIndex << " connected!" << std::endl;

				{
					ConnectionInfo connectionInfo;
					memset(&connectionInfo, 0, sizeof(connectionInfo));
					connectionInfo.IsConnected = true;
					connectionInfo.IpAddress = addr.sin_addr.S_un.S_addr;

					{
						std::lock_guard<std::recursive_mutex> guard(m_connectionsMutex);

						m_connections.insert(std::pair<SOCKET, ConnectionInfo>(
							connectionSocket,
							connectionInfo));

						m_slots[clientIndex] = connectionSocket;
					}
				}

				std::thread t(&Server::ClientHandlingThread, this, connectionSocket);
				t.detach();
			}
			else
			{
				Net::SendHeader(connectionSocket, Net::PacketType::RejectServerFull);
				closesocket(connectionSocket);
			}
		}
	}
}

