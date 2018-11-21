#pragma once

#include <WinSock2.h>

#include <mutex>
#include <thread> 

#include "Shared.h"

class LobbyClient
{
public:
	LobbyClient();
	~LobbyClient();

	bool ConnectToLobby(const char* pIpAddress, int port);
	bool HandleInput(const std::string& input);

	inline int GetNumPeers() { return m_numPeers; }
	bool GetPeer(int index, Net::ClientEndpoint& o_peer);
	inline SOCKET GetGameSocket() { return m_gameSocket; }

private:
	bool ReceiveMessage();
	void ServerRecieveThread();
	bool OpenGameplaySocket(u16& o_port);

	SOCKET m_lobbySocket = 0;
	SOCKET m_gameSocket = 0;
	std::thread m_receiveThread;
	Net::ClientEndpoint m_peers[Net::kNumSlots];
	int m_numPeers = -1;
};

