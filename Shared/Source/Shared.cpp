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

static bool SendAll(SOCKET socket, const char* pBuffer, size_t numBytes)
{
	size_t bytesSent = 0;
	while (bytesSent < numBytes)
	{
		int result = send(
			socket,
			(pBuffer + bytesSent),
			numBytes - bytesSent,
			0);

		if (result == SOCKET_ERROR)
		{
			return false;
		}

		bytesSent += result;
	}

	return true;
}

bool SendHeader(SOCKET socket, PacketType packetType)
{
	return SendAll(socket, (const char*)(&packetType), sizeof(packetType));
}

bool SendU32(SOCKET socket, u32 num)
{
	num = htonl(num);
	return SendAll(socket, (const char*)(&num), sizeof(num));
}

bool SendBuffer(SOCKET socket, const char* pBuffer, size_t size)
{
	if (size == 0)
		return false;

	bool result;

	result = SendU32(socket, size);
	if (!result)
		return false;

	result = SendAll(socket, pBuffer, size);
	if (!result)
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

static bool ReceiveAll(SOCKET socket, char* pBuffer, size_t numBytes)
{
	size_t bytesReceived = 0;
	while (bytesReceived < numBytes)
	{
		int result = recv(
			socket,
			(pBuffer + bytesReceived),
			(numBytes - bytesReceived),
			0);

		if (result == SOCKET_ERROR)
		{
			return false;
		}
		bytesReceived += result;
	}

	return true;
}

bool ReceiveU32(SOCKET socket, u32& num)
{
	bool result = ReceiveAll(socket, (char*)(&num), sizeof(num));
	if(result)
	{
		num = ntohl(num);
		return true;
	}
	else
	{
		return false;
	}
	
}

bool ReceiveHeader(SOCKET socket, PacketType& o_header)
{
	return ReceiveAll(socket, (char*)(&o_header), sizeof(o_header));
}

bool ReceiveBuffer(SOCKET socket, std::string& o_buffer)
{
	bool result;
	u32 bufferSize;
	result = ReceiveU32(socket, bufferSize);
	if (!result)
		return false;

	std::unique_ptr<char[]> pBuffer(new char[bufferSize]);
	result = ReceiveAll(socket, pBuffer.get(), bufferSize);
	if (!result)
		return false;

	o_buffer = std::string(pBuffer.get(), bufferSize);

	return true;
}

}
