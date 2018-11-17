#pragma once

#include <WinSock2.h>

#include <string>

#include "Types.h"

namespace Net
{

enum class PacketType : u8
{
	Message
};

void Init();

void SendHeader(SOCKET socket, PacketType packetType);
void SendBuffer(SOCKET socket, const char* pBuffer, size_t size);
void SendBuffer(SOCKET socket, const std::string& message);

void SendMessagePacket(SOCKET socket, const char* pBuffer, size_t size);
void SendMessagePacket(SOCKET socket, const std::string& message);

PacketType ReceiveHeader(SOCKET socket);
std::string ReceiveBuffer(SOCKET socket);

}

