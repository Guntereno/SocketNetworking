#include "pch.h"

#include "Shared.h"

#include "Server.h"

int main()
{
	Net::Init();

	Server server;
	bool result = server.Listen(1111, false);
	
	if (result)
	{
		// Keep going until all users are ready
		bool shouldContinue = true;
		do
		{
			shouldContinue = !server.CheckForReadyState();
		} while (shouldContinue);
	}

	system("pause");

	return result ? 0 : 1;
}
