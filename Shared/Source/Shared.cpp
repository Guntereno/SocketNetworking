#include "Shared.h"

namespace Net
{

void Init()
{
	WSAData wsaData;
	WORD d11Version = MAKEWORD(2, 1);
	if (WSAStartup(d11Version, &wsaData) != 0)
	{
		MessageBoxA(nullptr, "Failed to start Winsock", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}
}

void SendBuffer(SOCKET socket, const char* pBuffer, size_t size)
{
	if (size == 0)
		return;

	send(socket, (const char*)(&size), sizeof(size), 0);
	send(socket, pBuffer, size, 0);
}

void SendBuffer(SOCKET socket, const std::string& message)
{
	SendBuffer(socket, message.c_str(), message.length());
}

std::string ReceiveBuffer(SOCKET socket)
{
	size_t bufferSize;
	recv(socket, (char*)(&bufferSize), sizeof(bufferSize), 0);

	std::unique_ptr<char[]> pBuffer(new char[bufferSize]);
	recv(socket, pBuffer.get(), bufferSize, 0);

	return std::string(pBuffer.get(), bufferSize);
}

}
