#include "pch.h"

#include "Shared.h"

#include "Client.h"
#include <iostream>

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

	Client client;

	bool result = client.ConnectToLobby(pServer, kDefaultPort);
	if (!result)
	{
		return 1;
	}

	std::string buffer;
	bool shouldContinue = true;
	do
	{
		std::getline(std::cin, buffer);
		shouldContinue = client.HandleInput(buffer);
	} while (shouldContinue);

	std::cerr << "Lost connection to server." << std::endl;
	
	system("pause");

	return 1;
}
