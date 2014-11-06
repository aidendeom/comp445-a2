#pragma once

#pragma comment( linker, "/defaultlib:ws2_32.lib" )

#include <winsock2.h>
#include <string>

#include "../Utils/Utils.h"

class Server
{
public:
	Server();
	~Server();

	void run();

private:
	SOCKET s;
	SOCKADDR_IN sa_out;
	std::string router_hostname;
	int connectionAckNo;
	int currentSeqNo;
	int expectedSeqNo;

	void threeWayHandshake();
	void sendPacket(const Packet& p);
	void recvFile();
	void recvPacket(Packet& p);
	bool recvPacketWithACK(Packet& p);
};

