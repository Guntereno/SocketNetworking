#pragma once

#include <WinSock2.h>

#include <string>

#include "Types.h"

namespace Net
{

enum class PacketType : u8
{
	Message,
	ConnectTo,
	RejectServerFull,
	RequestOpenUdp,
	AcknowledgeOpenUdp,
	Ready,
	StartGame,
	ClientRecipient,
	ClientValue,
	ClientNull
};

struct ClientEndpoint
{
	u32 IpAddress;
	u16 Port;
};

void Init();

bool SendHeader(SOCKET socket, PacketType packetType);
bool SendU32(SOCKET socket, u32 num);
bool SendU16(SOCKET socket, u16 num);
bool SendBuffer(SOCKET socket, const char* pBuffer, int size);
bool SendBuffer(SOCKET socket, const std::string& message);

bool SendMessagePacket(SOCKET socket, const char* pBuffer, int size);
bool SendMessagePacket(SOCKET socket, const std::string& message);

bool ReceiveU32(SOCKET socket, u32& num);
bool ReceiveU16(SOCKET socket, u16& num);
bool ReceiveHeader(SOCKET socket, PacketType& o_header);
bool ReceiveBuffer(SOCKET socket, std::string& o_buffer);

void OutputIp(std::ostream& stream, u32 ip);

const int kNumSlots = 4;


}

