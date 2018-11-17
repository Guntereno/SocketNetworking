#pragma once

#include <WinSock2.h>

#include <string>

namespace Net
{
	void Init();
	void SendBuffer(SOCKET socket, const char* pBuffer, size_t size);
	void SendBuffer(SOCKET socket, const std::string& message);
	std::string ReceiveBuffer(SOCKET socket);
}

