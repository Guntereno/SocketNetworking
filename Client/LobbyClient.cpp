// Client

#include "pch.h"

#include "LobbyClient.h"

#pragma comment(lib, "Ws2_32.lib")

#include "Shared.h"

#include <ws2tcpip.h>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>

LobbyClient::LobbyClient() :
	m_lobbySocket(-1)
{}

LobbyClient::~LobbyClient()
{
	closesocket(m_lobbySocket);
}

bool LobbyClient::ConnectToLobby(const char* pIpAddress, int port)
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
	m_lobbySocket = socket(AF_INET, SOCK_STREAM, 0);

	result = connect(m_lobbySocket, (SOCKADDR*)(&addr), addrLen);
	if (result != 0)
	{
		std::cout << "Failed to connect to server! " << WSAGetLastError() << std::endl;
		return false;
	}

	std::thread t(&LobbyClient::ServerRecieveThread, this);
	t.detach();

	return true;
}

bool LobbyClient::HandleInput(const std::string& input)
{
	// Commands begin ':', everything else is treated as a message

	if (input.c_str()[0] == ':')
	{
		std::stringstream stringStream(input);
		std::string command;
		std::getline(stringStream, command);
		if (command == ":ready")
		{
			bool result = Net::SendHeader(m_lobbySocket, Net::PacketType::Ready);

			// Stop capturing input when the client is 'ready'
			return false;
		}
		else
		{
			std::cerr << "Unhandled command:" << command << std::endl;

			// Though this is an error, we don't want to cause a disconnect so...
			return true;
		}
	}
	else
	{
		return Net::SendMessagePacket(m_lobbySocket, input);
	}
}

bool LobbyClient::GetPeer(int index, Net::ClientEndpoint& o_peer)
{
	if ((index >= 0) && (index < m_numPeers))
	{
		o_peer = m_peers[index];
		return true;
	}
	else
	{
		std::cerr << "Index " << index << " is out of bounds!" << std::endl;
		return false;
	}
}

bool LobbyClient::ReceiveMessage()
{
	bool result;

	std::string message;
	result = Net::ReceiveBuffer(m_lobbySocket, message);

	if (result)
	{
		std::cout << message << std::endl;
		return true;
	}

	return false;
}

void LobbyClient::ServerRecieveThread()
{
	bool hasConnection = true;
	bool shouldContinue = true;
	do
	{
		Net::PacketType type;
		hasConnection = Net::ReceiveHeader(m_lobbySocket, type);

		if (hasConnection)
		{
			switch (type)
			{
				case Net::PacketType::Message:
				{
					hasConnection = ReceiveMessage();
				}
				break;

				case Net::PacketType::RejectServerFull:
				{
					std::cout << "Connection has been rejected as the server is full." << std::endl;;
					shouldContinue = false;
				}
				break;

				case Net::PacketType::RequestOpenUdp:
				{
					u16 port = 0;
					hasConnection = OpenGameplaySocket(port);
					hasConnection = hasConnection ? Net::SendHeader(m_lobbySocket, Net::PacketType::AcknowledgeOpenUdp) : false;
					hasConnection = hasConnection ? Net::SendU16(m_lobbySocket, port) : false;
					if (!hasConnection)
					{
						std::cerr << "Failed to send open UDP acknowledgment: " << WSAGetLastError() << std::endl;
					}
				}
				break;

				case Net::PacketType::StartGame:
				{
					std::cout << "Game has started." << std::endl;

					std::cout << "Clients:" << std::endl;
					int numPeers = 0;
					for (int i = 0; i < Net::kNumSlots; ++i)
					{
						bool result;

						Net::PacketType header;
						result = Net::ReceiveHeader(m_lobbySocket, header);
						if (result)
						{
							switch (header)
							{
								case Net::PacketType::ClientRecipient:
								{
									std::cout << i << " is you." << std::endl;
								}
								break;

								case Net::PacketType::ClientValue:
								{
									u32 ipAddress;
									result = Net::ReceiveU32(m_lobbySocket, ipAddress);

									u16 port;
									result = Net::ReceiveU16(m_lobbySocket, port);

									std::cout << i << " is ";
									Net::OutputIp(std::cout, ipAddress);
									std::cout << ":" << port << std::endl;

									m_peers[numPeers++] = { ipAddress, port };
								}
								break;

								case Net::PacketType::ClientNull:
								{
									std::cout << i << " is " << "NULL" << std::endl;
								}
								break;

								default:
								{
									std::cerr << "Invalid client type: " << (int)header;
								}
								break;
							}
						}
					}

					m_numPeers = numPeers;

					shouldContinue = false;
				}
				break;

				default:
				{
					std::cerr << "Unhandled packet type!: " << (int)type << std::endl;
					hasConnection = false;
				}
			}
		}

		if (!hasConnection)
		{
			std::cerr << "Lost connection to server." << std::endl;
			shouldContinue = false;
		}

	} while (shouldContinue);

	closesocket(m_lobbySocket);
}

bool LobbyClient::OpenGameplaySocket(u16& o_port)
{
	int address_family = AF_INET;
	int type = SOCK_DGRAM;
	int protocol = IPPROTO_UDP;
	m_gameSocket = socket(address_family, type, protocol);

	if (m_gameSocket == INVALID_SOCKET)
	{
		std::cerr << "Failed to create UDP socket: " << WSAGetLastError() << std::endl;;
		return false;
	}

	const int kBaseGameplayPort = 2222;
	const int kRangeWidth = 100;
	bool success = false;

	for (int port = kBaseGameplayPort; port < (kBaseGameplayPort + kRangeWidth); ++port)
	{
		SOCKADDR_IN localAddress;
		localAddress.sin_family = AF_INET;
		localAddress.sin_port = htons(port);
		localAddress.sin_addr.s_addr = INADDR_ANY;
		int result = bind(m_gameSocket, (SOCKADDR*)&localAddress, sizeof(localAddress));
		if (result == SOCKET_ERROR)
		{
			int wsaError = WSAGetLastError();
			if (wsaError == WSAEADDRINUSE)
				continue;

			std::cerr << "Failed to bind UDP socket to port " << port << ": " << wsaError << std::endl;
		}
		else
		{
			std::cout << "Opened UDP socket on port " << port << "." << std::endl;
			success = true;
			o_port = port;
			break;
		}
	}

	if (!success)
	{
		std::cerr << "Failed to bind UDP socket to any port!: " << WSAGetLastError() << std::endl;
	}

	return success;
}
