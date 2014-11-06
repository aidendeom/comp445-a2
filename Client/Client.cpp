#include "Client.h"

#include <iostream>

#include "../Router/Router.h"
#include "../Utils/Utils.h"

Client::Client() :
	enableLogging{ false }
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
		// Keep trying to connect to server
		threeWayHandshake();

		//sendFile();
		recvFile();
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

	int synNumber;
	
	bool sent = false;

	while (!sent)
	{
		synNumber = rand();
		p.syn = true;
		p.seqNo = synNumber;
		int expectedAckNo = synNumber + 1;

		printf_s("Sending SYN %d\n", p.seqNo);
		sendPacket(p);

		FD_ZERO(&readfds);
		FD_SET(s, &readfds);

		if (select(1, &readfds, nullptr, nullptr, &tp) == 0)
		{
			log("Timeout during handshake waiting for SYN+ACK\n");
		}
		else
		{
			currentSeqNo = (~p.seqNo) & 1;
			recvPacket(p);

			if (p.ackNo == expectedAckNo)
			{
				sent = true;
				expectedSeqNo = (~p.seqNo) & 1;
				printf_s("Received SYN/ACK packet: SYN -> %d ACK -> %d\n", p.seqNo, p.ackNo);
			}
			else
			{
				printf_s("Received AckNo %d, expected %d\n", p.ackNo, expectedAckNo);
			}
		}
	}

	p.ackNo = p.seqNo + 1;

	printf_s("Sending final ACK %d\n", p.ackNo);
	connectionAck = p.ackNo;
	sendPacket(p);
	std::cout << "Handshake success!" << std::endl << std::flush;
	printf_s("Current SeqNo %d\nExpected SeqNo %d\n", currentSeqNo, expectedSeqNo);
}

void Client::sendFile()
{
	std::ifstream file;
	std::string filename;
	do
	{
		filename = selectLocalFile();
		file.open(filename, std::ios::binary | std::ios::ate);
	} while (!file.is_open());

	size_t filesize = static_cast<size_t>(file.tellg());
	file.seekg(0);

	Packet p;
	p.seqNo = currentSeqNo;
	sprintf_s(p.data, filename.length() + 1, filename.c_str());
	sendPacketWithACK(p);

	sendFile(file, filesize);
}

void Client::sendFile(std::ifstream& file, size_t filesize)
{
	Packet p;

	size_t numBuffs = (filesize / Packet::DATA_LENGTH) + 1;
	size_t remainder = filesize % Packet::DATA_LENGTH;

	for (size_t i = 0; i < numBuffs; i++)
	{
		size_t length = i == numBuffs - 1 ? remainder : Packet::DATA_LENGTH;

		file.read(p.data, length);
		p.length = length;
		p.seqNo = currentSeqNo;

		sendPacketWithACK(p);
	}

	std::cout << "Done sending file" << std::endl;
}

std::string Client::selectLocalFile()
{
	std::cout << "Local file listing:" << std::endl;
	std::cout << getDirectoryItems() << std::endl;
	std::cout << "Select a file: ";
	std::string filename;
	std::cin >> filename;
	return filename;
}

void Client::recvFile()
{
	Packet p;

	std::cout << "Select remote file to GET: ";
	std::string filename;
	std::cin >> filename;

	p.seqNo = currentSeqNo;
	sprintf_s(p.data, filename.length() + 1, filename.c_str());
	sendPacketWithACK(p);

	std::ofstream file(filename, std::ios::binary | std::ios::trunc);

	do
	{
		while (!recvPacketWithACK(p));
		file.write(p.data, p.length);
	} while (p.length == Packet::DATA_LENGTH);

	std::cout << "Receive done!";
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
			printf_s("Sending packet SeqNo = %d\n", p.seqNo);
			sendPacket(p);

			if (select(1, &readfds, nullptr, nullptr, &tp) == 0)
			{
				throw "Timeout while waiting for response! Resending...\n";
			}
			else
			{
				recvPacket(response);
				printf_s("Received response. AckNo = %d, expected = %d\n", response.ackNo, expectedAckNo);
				if (response.ackNo == expectedAckNo)
				{
					sent = true;
					//expectedSeqNo ^= 1;
					currentSeqNo ^= 1;
				}
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

bool Client::recvPacketWithACK(Packet& p)
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

void Client::log(const char *str)
{
	std::cout << str << std::endl << std::flush;
}

int main()
{
	Client c;
	c.run();
	return 0;
}