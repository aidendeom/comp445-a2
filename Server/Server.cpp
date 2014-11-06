#include "Server.h"

#include <iostream>

#include "../Router/Router.h"
#include "../Utils/Utils.h"

Server::Server()
{
	std::cout << "=== SERVER ===" << std::endl << std::flush;

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
		sa_in.sin_port = htons(PEER_PORT_SERVER);
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
		sa_out.sin_port = htons(ROUTER_PORT_SERVER);

		srand((unsigned)time(nullptr));

		std::cout << "Server ready" << std::endl << std::flush;
	}
	catch (const char *str)
	{
		std::cout << str << WSAGetLastError() << std::endl << std::flush;
	}
}


Server::~Server()
{
	closesocket(s);
	WSACleanup();
}

void Server::run()
{
	Packet p;

	try
	{
		// Keep waiting for a connection
		bool connected = false;
		while (!connected)
		{
			try
			{
				threeWayHandshake();
				connected = true;
			}
			catch (const char *str)
			{
				printf_s(str);
			}
		}

		// Connected
		recvFile();
	}
	catch (const char* str)
	{
		std::cout << str << WSAGetLastError() << std::endl << std::flush;
	}
}

void Server::threeWayHandshake()
{
	Packet p;

	timeval tp;
	tp.tv_sec = 0;
	tp.tv_usec = TIMEOUT_USEC;

	fd_set readfds;

	recvPacket(p);
	printf_s("Received SYN packet: %d\n", p.seqNo, p.ackNo);

	expectedSeqNo = (!p.seqNo) & 1;

	p.ackNo = p.seqNo + 1;
	int num = rand();
	p.seqNo = num;
	p.syn = true;

	bool sent = false;

	while (!sent)
	{
		printf_s("Sending SYN/ACK packet: SYN -> %d ACK -> %d\n", p.seqNo, p.ackNo);
		sendPacket(p);

		FD_ZERO(&readfds);
		FD_SET(s, &readfds);

		if (select(1, &readfds, nullptr, nullptr, &tp) == 0)
		{
			printf_s("Timeout in handshake waiting for final ACK\n");
		}
		else
		{
			sent = true;
			recvPacket(p);
			printf_s("Received final ACK packet: %d\n", p.ackNo);

			connectionAckNo = num + 1;

			if (p.ackNo != connectionAckNo)
				throw "Three way handshake failed\n";

			std::cout << "Handshake success!" << std::endl << std::flush;
		}
	}
}

void Server::recvFile()
{
	Packet p;

	std::ofstream file("received_file.pdf", std::ios::binary | std::ios::trunc);

	do
	{
		while (!recvPacketWithACK(p));
		file.write(p.data, p.length);
	} while (p.length == Packet::DATA_LENGTH);

	std::cout << "Receive done!";
}

bool Server::recvPacketWithACK(Packet& p)
{
	Packet response;

	recvPacket(p);
	printf_s("Received packet, SeqNo = %d\n", p.seqNo);

	if (p.seqNo == expectedSeqNo)
	{
		response.ackNo = p.seqNo;
		sendPacket(response);
		printf_s("Sending response, AckNo = %d\n", response.ackNo);

		expectedSeqNo ^= 1;

		return true;
	}
	else
	{
		printf_s("Unexpectected SeqNo %d, sending ACK\n", p.seqNo);
		response.ackNo = p.seqNo;
		sendPacket(response);
		return false;
	}
}

void Server::sendPacket(const Packet& p)
{
	const int len = sizeof(p);
	char buffer[len];
	memcpy(buffer, &p, len);
	if (sendto(s, buffer, len, 0, (SOCKADDR*)&sa_out, sizeof(sa_out)) == SOCKET_ERROR)
		throw "Send fail\n";
}

void Server::recvPacket(Packet& p)
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
	Server s;
	s.run();
	return 0;
}