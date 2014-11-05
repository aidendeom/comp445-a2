#include "Client.h"

#include <iostream>

#include "../Router/Router.h"
#include "../Utils/Utils.h"

Client::Client()
{
	std::cout << "=== CLIENT ===" << std::endl << std::flush;
	try
	{
		// Initialize WinSock
		WSADATA wsadata;

		if (WSAStartup(0x0202, &wsadata) != 0)
			throw "Error in starting WSAStartup()\n";

		std::cout << std::endl
			<< "wsadata.wVersion " << wsadata.wVersion << std::endl
			<< "wsadata.wHighVersion " << wsadata.wHighVersion << std::endl
			<< "wsadata.szDescription " << wsadata.szDescription << std::endl
			<< "wsadata.szSystemStatus " << wsadata.szSystemStatus << std::endl
			<< "wsadata.iMaxSockets " << wsadata.iMaxSockets << std::endl
			<< "wsadata.iMaxUdpDg " << wsadata.iMaxUdpDg << std::endl;

		// Create socket
		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == SOCKET_ERROR)
			throw "Socket creation failed\n";

		//Fill-in UDP Port and Address info.
		SOCKADDR_IN sa_in;
		sa_in.sin_family = AF_INET;
		sa_in.sin_port = htons(PEER_PORT_CLIENT);
		sa_in.sin_addr.s_addr = htonl(INADDR_ANY);

		//Bind the UDP port
		if (bind(s, (LPSOCKADDR)&sa_in, sizeof(sa_in)) == SOCKET_ERROR)
			throw "Socket bind failed\n";

		std::cout << "Enter router hostname: " << std::flush;
		std::cin >> router_hostname;
		std::cout << std::endl << std::flush;

		HOSTENT *hp;
		if ((hp = gethostbyname(router_hostname.c_str())) == NULL)
			throw "get server name failed\n";
		memset(&sa_out, 0, sizeof(sa_out));
		memcpy(&sa_out.sin_addr, hp->h_addr, hp->h_length);
		sa_out.sin_family = hp->h_addrtype;
		sa_out.sin_port = htons(ROUTER_PORT_CLIENT);

		srand((unsigned)time(nullptr));

		std::cout << "Client ready" << std::endl << std::flush;
	}
	catch (const char *str)
	{
		std::cout << str << WSAGetLastError() << std::endl << std::flush;
	}
}


Client::~Client()
{
	closesocket(s);
	WSACleanup();
}

void Client::run()
{
	Packet p;

	try
	{
		//threeWayHandshake();
		p.seqNo = 0;
		sprintf_s(p.data, "Hello, world!");
		sendPacketWithACK(p);

		p.seqNo = 1;
		sprintf_s(p.data, "This is the second message");
		sendPacketWithACK(p);
	}
	catch (const char* str)
	{
		std::cout << str << WSAGetLastError() << std::endl << std::flush;
	}
}

void Client::threeWayHandshake()
{
	Packet p;

	timeval tp;
	tp.tv_sec = 0;
	tp.tv_usec = TIMEOUT_USEC;

	fd_set readfds;

	int num = rand();
	p.seqNo = num;

	bool sent = false;

	while (!sent)
	{
		sendPacket(p);

		FD_ZERO(&readfds);
		FD_SET(s, &readfds);

		if (select(1, &readfds, nullptr, nullptr, &tp) == 0)
		{
			printf_s("Timeout hanshake, resend\n");
			continue;
		}
		else
		{
			recvPacket(p);
			sent = true;
		}
	}

	if (p.ackNo != num + 1)
		throw "Three way handshake failed\n";

	p.ackNo = p.seqNo + 1;

	sendPacket(p);

	std::cout << "Handshake success!" << std::endl << std::flush;
}

void Client::sendPacketWithACK(const Packet& p)
{
	int expectedAckNo = p.seqNo;

	Packet response;

	timeval tp;
	tp.tv_sec = 0;
	tp.tv_usec = TIMEOUT_USEC;

	fd_set readfds;

	bool sent = false;

	while (!sent)
	{
		try
		{
			// Set which sockets to watch
			FD_ZERO(&readfds);
			FD_SET(s, &readfds);

			// TODO: Optimize so that it doesn't recopy to the buffer on a dropped packet
			sendPacket(p);

			if (select(1, &readfds, nullptr, nullptr, &tp) == 0)
			{
				throw "Timeout while waiting for response! Resending...\n";
			}
			else
			{
				recvPacket(response);
				if (response.ackNo == expectedAckNo)
					sent = true;
				else
				{
					throw "Response ACK number not equal! Resending...\n";
				}
			}
		}
		catch (const char *str)
		{
			std::cout << str << WSAGetLastError() << std::endl << std::flush;
			// TODO: Log the error
		}
	}
}

void Client::sendPacket(const Packet& p)
{
	const int len = sizeof(p);
	char buffer[len];
	memcpy(buffer, &p, len);
	if (sendto(s, buffer, len, 0, (SOCKADDR*)&sa_out, sizeof(sa_out)) == SOCKET_ERROR)
		throw "Send fail\n";
}

void Client::recvPacket(Packet& p)
{
	const int len = sizeof(p);
	int recvLen;
	char buffer[len];
	SOCKADDR from;
	int fromLen = sizeof(from);
	if ((recvLen = recvfrom(s, buffer, len, 0, &from, &fromLen)) == SOCKET_ERROR)
		throw "Recieve failed\n";

	memcpy(&p, buffer, len);
}

int main()
{
	Client c;
	c.run();
	return 0;
}