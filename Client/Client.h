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
	bool enableLogging;
	int connectionAck;
	int currentSeqNo;
	int expectedSeqNo;

	void threeWayHandshake();
	void sendPacket(const Packet& p);
	void recvPacket(Packet& p);
	void sendPacketWithACK(const Packet& p);
	bool recvPacketWithACK(Packet& p);
	void sendFile();
	void sendFile(std::ifstream& file, size_t filesize);
	std::string selectLocalFile();
	void recvFile();
	void log(const char *str);
};

