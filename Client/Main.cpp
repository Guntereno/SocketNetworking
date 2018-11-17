#include "pch.h"

#include "Shared.h"

#include "Client.h"
#include <iostream>

void main()
{
	Net::Init();

	Client client;

	client.Connect("127.0.0.1", 1111);

	std::string buffer;
	bool shouldContinue = true;
	do
	{
		std::getline(std::cin, buffer);
		shouldContinue = client.HandleInput(buffer);
	} while (shouldContinue);

	std::cerr << "Lost connection to server." << std::endl;
	
	system("pause");
}
