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

bool SendHeader(SOCKET socket, PacketType packetType)
{
	int result;
	result = send(socket, (const char*)(&packetType), sizeof(packetType), 0);
	if (result == SOCKET_ERROR)
		return false;

	return true;
}

bool SendBuffer(SOCKET socket, const char* pBuffer, size_t size)
{
	if (size == 0)
		return false;

	int result;
	result = send(socket, (const char*)(&size), sizeof(size), 0);
	if (result == SOCKET_ERROR)
		return false;

	send(socket, pBuffer, size, 0);
	if (result == SOCKET_ERROR)
		return false;

	return true;
}

bool SendMessagePacket(SOCKET socket, const char* pBuffer, size_t size)
{
	bool result = SendHeader(socket, PacketType::Message);
	result = result ? SendBuffer(socket, pBuffer, size) : false;
	return result;
}

bool SendMessagePacket(SOCKET socket, const std::string& message)
{
	bool result = SendHeader(socket, PacketType::Message);
	result = result ? SendBuffer(socket, message) : false;
	return result;
}

bool SendBuffer(SOCKET socket, const std::string& message)
{
	return SendBuffer(socket, message.c_str(), message.length());
}

bool ReceiveHeader(SOCKET socket, PacketType& o_header)
{
	int result;
	result = recv(socket, (char*)(&o_header), sizeof(o_header), 0);
	if (result == SOCKET_ERROR)
		return false;

	return true;
}

bool ReceiveBuffer(SOCKET socket, std::string& o_buffer)
{
	int result;
	size_t bufferSize;
	result = recv(socket, (char*)(&bufferSize), sizeof(bufferSize), 0);
	if (result == SOCKET_ERROR)
		return false;

	std::unique_ptr<char[]> pBuffer(new char[bufferSize]);
	result = recv(socket, pBuffer.get(), bufferSize, 0);
	if (result == SOCKET_ERROR)
		return false;

	o_buffer = std::string(pBuffer.get(), bufferSize);
	return true;
}

}
