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

bool SendHeader(SOCKET socket, PacketType packetType);
bool SendU32(SOCKET socket, u32 num);
bool SendBuffer(SOCKET socket, const char* pBuffer, size_t size);
bool SendBuffer(SOCKET socket, const std::string& message);

bool SendMessagePacket(SOCKET socket, const char* pBuffer, size_t size);
bool SendMessagePacket(SOCKET socket, const std::string& message);

bool ReceiveU32(SOCKET socket, u32& num);
bool ReceiveHeader(SOCKET socket, PacketType& o_header);
bool ReceiveBuffer(SOCKET socket, std::string& o_buffer);

}

