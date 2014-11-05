#pragma once

#pragma comment( linker, "/defaultlib:ws2_32.lib" )

#include <winsock2.h>
#include <string>

#include "../Utils/Utils.h"

class Client
{
public:
	Client();
	~Client();

	void run();

private:
	SOCKET s;
	SOCKADDR_IN sa_out;
	std::string router_hostname;
	int connectionAckNo;

	// Returns true if connection has been made.
	bool threeWayHandshake();
	void sendPacket(const Packet& p);
	void recvPacket(Packet& p);
	void sendPacketWithACK(const Packet& p);
};

