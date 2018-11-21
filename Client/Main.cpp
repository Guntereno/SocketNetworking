#include "pch.h"

#include "Shared.h"
#include "LobbyClient.h"

#include <iostream>
#include <iomanip>
#include <mutex>

#define _USE_MATH_DEFINES
#include <math.h>

#include <windows.h>


// we use a fixed timestep of 1 / (60 fps) = 16 milliseconds
constexpr std::chrono::nanoseconds timestep(16000000);
static const int kSocketBufferSize = 256;

struct GameState
{
	int frameNum = 0;
	float myValue = 0.0f;
	float peerValues[Net::kNumSlots];
};

int g_numPeers = 0;
Net::ClientEndpoint g_peers[Net::kNumSlots];


struct PeerData
{
	int frame;
	float value;
};
PeerData g_clientRecieveBuffers[Net::kNumSlots];
std::mutex g_clientReceiveMutex;

SOCKET g_udpSocket = INVALID_SOCKET;


bool JoinLobby(const char* pServer, const int kDefaultPort)
{
	LobbyClient client;

	bool result = client.ConnectToLobby(pServer, kDefaultPort);
	if (!result)
	{
		return false;
	}

	std::string buffer;
	bool shouldContinue = true;
	do
	{
		std::getline(std::cin, buffer);
		shouldContinue = client.HandleInput(buffer);
	} while (shouldContinue);

	while (client.GetNumPeers() < 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	std::cerr << "Lost connection to server." << std::endl;

	g_numPeers = client.GetNumPeers();
	for (int i = 0; i < g_numPeers; ++i)
	{
		client.GetPeer(i, g_peers[i]);
	}
	g_udpSocket = client.GetGameSocket();

	return true;
}


void GameUpdate(GameState& gameState)
{
	const double kWaveFreq = 0.5;
	double theta = (M_PI * 2.0) * ((double)gameState.frameNum / 60);
	gameState.myValue = (float)(sin(theta));

	gameState.frameNum++;
}

float Interpolate(float a, float b, float t)
{
	return (((1.0f - t) * a) + (t * b));
}

GameState Interpolate(const GameState& previous, const GameState& current, float t)
{
	GameState result;
	result.myValue = Interpolate(previous.myValue, current.myValue, t);

	for (int i = 0; i < g_numPeers; ++i)
	{
		result.peerValues[i] = Interpolate(previous.peerValues[i], current.peerValues[i], t);
	}

	return result;
}

void setCursorPosition(int x, int y)
{
	static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	std::cout.flush();
	COORD coord = { (SHORT)x, (SHORT)y };
	SetConsoleCursorPosition(hOut, coord);
}

void Cls()
{
	setCursorPosition(0, 0);
}

void ShowConsoleCursor(bool showFlag)
{
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_CURSOR_INFO     cursorInfo;

	GetConsoleCursorInfo(out, &cursorInfo);
	cursorInfo.bVisible = showFlag; // set the cursor visibility
	SetConsoleCursorInfo(out, &cursorInfo);
}

void Render(const GameState& state)
{
	Cls();
	std::cout << std::fixed << std::setprecision(6) << std::setfill(' ');
	std::cout << std::noshowpos << "0:" << std::showpos << state.myValue << std::endl;
	for (int i = 0; i < g_numPeers; ++i)
	{
		std::cout << std::noshowpos << (i + 1) << ":" << std::showpos << state.peerValues[i] << std::endl;;
	}
	std::cout.flush();
}

bool SendState(const GameState& gameState)
{
	char buffer[kSocketBufferSize];

	int writePos = 0;

	// TODO: Not dealing with endian changes here! Ideally would use a serialization system.
	// (as all clients are presently on Windows, this shouldn't be an issue.)
	memcpy(buffer + writePos, &(gameState.myValue), sizeof(gameState.frameNum));
	writePos += sizeof(gameState.frameNum);
	memcpy(buffer + writePos, &(gameState.myValue), sizeof(gameState.myValue));
	writePos += sizeof(gameState.myValue);

	bool success = true;
	for (int i = 0; i < g_numPeers; ++i)
	{
		SOCKADDR_IN peerAddress;
		peerAddress.sin_family = AF_INET;
		peerAddress.sin_port = htons(g_peers[i].Port);
		peerAddress.sin_addr.S_un.S_addr = g_peers[i].IpAddress;

		int result = sendto(g_udpSocket, buffer, writePos, 0, (SOCKADDR*)(&peerAddress), sizeof(peerAddress));
		if (result == SOCKET_ERROR)
		{
			std::cerr << "sendto failed sending to peer " << i << ": " << WSAGetLastError(); 
			success = false;
			break;
		}
	}

	return success;
}

int GetPeerIndexWithEndpoint(u32 ip, u16 port)
{
	for (int i = 0; i < g_numPeers; ++i)
	{
		if ((g_peers[i].IpAddress == ip) && (g_peers[i].Port == port))
		{
			return i;
		}
	}

	return -1;
}

void PeerReceiveThread()
{
	char buffer[kSocketBufferSize];

	bool shouldContinue = true;
	while (true)
	{
		int flags = 0;
		SOCKADDR_IN from;
		int addrSize = sizeof(from);
		int bytes_received = recvfrom(g_udpSocket, buffer, kSocketBufferSize, flags, (SOCKADDR*)&from, &addrSize);

		if (bytes_received == SOCKET_ERROR)
		{
			std::cerr << "recvfrom returned SOCKET_ERROR, WSAGetLastError(): " << WSAGetLastError() << std::endl;
		}
		else
		{
			ULONG ip = from.sin_addr.S_un.S_addr;
			USHORT port = ntohs(from.sin_port);
			int peerIndex = GetPeerIndexWithEndpoint(ip, port);

			if (peerIndex >= 0)
			{
				std::lock_guard<std::mutex> guard(g_clientReceiveMutex);

				u32 readIndex = 0;

				PeerData& peerData = g_clientRecieveBuffers[peerIndex];

				// Note: Not handling endianness here
				memcpy(&(peerData.frame), buffer + readIndex, sizeof(peerData.frame));
				readIndex += sizeof(peerData.frame);

				memcpy(&(peerData.value), buffer + readIndex, sizeof(peerData.value));
				readIndex += sizeof(peerData.value);
			}
			else
			{
				std::cerr << "Received packet from unknown peer: ";
				Net::OutputIp(std::cerr, ip);
				std::cerr << ":" << port << std::endl;
			}

		}
	}
}

bool GameLoop()
{
	std::thread t(PeerReceiveThread);
	t.detach();

	system("cls");
	ShowConsoleCursor(false);

	// Timer
	using clock = std::chrono::high_resolution_clock;
	std::chrono::nanoseconds lag(0);
	clock::time_point timeStart = clock::now();

	// Game state
	GameState currentState;
	memset(&currentState, 0, sizeof(currentState));
	GameState previousState = currentState;

	bool quit = false;
	while (!quit)
	{
		auto deltaTime = clock::now() - timeStart;
		timeStart = clock::now();
		lag += std::chrono::duration_cast<std::chrono::nanoseconds>(deltaTime);

		// update game logic as lag permits
		while (lag >= timestep)
		{
			lag -= timestep;

			previousState = currentState;
			GameUpdate(currentState); // update at a fixed rate each time

			{
				// Get the latest data received from the network
				std::lock_guard<std::mutex> guard(g_clientReceiveMutex);
				for (int i = 0; i < g_numPeers; ++i)
				{
					currentState.peerValues[i] = g_clientRecieveBuffers[i].value;
				}
			}

			// Send the state to the peers over the network
			bool result = SendState(currentState);

			if (!result)
			{
				quit = true;
				break;
			}
		}

		// calculate how close or far we are from the next timestep
		auto t = (float)lag.count() / timestep.count();
		auto interpolatedState = Interpolate(previousState, currentState, t);

		// Render
		Render(interpolatedState);
	}

	return true;
}


int main(int argc, char *argv[])
{ 
	const char* const kDefaultServer = "127.0.0.1";
	const int kDefaultPort = 1111;

	const char* pServer = kDefaultServer;
	if (argc > 1)
	{
		pServer = argv[1];
	}

	Net::Init();

	bool result;
	
	result = JoinLobby(pServer, kDefaultPort);

	if (!result)
		return 1;

	memset(&g_clientRecieveBuffers, 0, sizeof(g_clientRecieveBuffers));

	result = GameLoop();
	system("pause");

	return result ? 0 : 1;
}
