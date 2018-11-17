#include "pch.h"

#include "Shared.h"

#include "Server.h"

int main()
{
	Net::Init();

	Server server;
	bool result = server.Listen(1111, false);

	while (true)
	{}

	return result ? 0 : 1;
}
