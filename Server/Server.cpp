#include "Server.h"

#include <iostream>
#include <map>
#include <functional>

#include "../Router/Router.h"
#include "../Utils/Utils.h"
#include "../Utils/log.h"

Server::Server(bool debug) :
enableLogging{ debug },
packetCount{ 0 }
{
	std::cout << "=== SERVER ===" << std::endl << std::flush;

	try
	{
		// Initialize WinSock
		WSADATA wsadata;

		if (WSAStartup(0x0202, &wsadata) != 0)
			throw "Error in starting WSAStartup()\n";

		if (enableLogging)
		{
			FILELog::ReportingLevel() = logDEBUG;
			FILE *log_fd = fopen("server_log.txt", "w");
			Output2FILE::Stream() = log_fd;
		}
		else
		{
			FILELog::ReportingLevel() = logINFO;
		}

		char localhost[256];
		gethostname(localhost, 256);
		local_hostname = localhost;
		FILE_LOG(logDEBUG) << "Server started on " << local_hostname;

		// Create socket
		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == SOCKET_ERROR)
			throw "Socket creation failed\n";

		//Fill-in UDP Port and Address info.
		SOCKADDR_IN sa_in;
		sa_in.sin_family = AF_INET;
		sa_in.sin_port = htons(PEER_PORT_SERVER);
		sa_in.sin_addr.s_addr = htonl(INADDR_ANY);

		//Bind the UDP port
		if (::bind(s, (LPSOCKADDR)&sa_in, sizeof(sa_in)) == SOCKET_ERROR)
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

		std::cout << "Server ready" << std::endl;
		FILE_LOG(logDEBUG) << "Server ready";
	}
	catch (const char *str)
	{
		std::cout << str << WSAGetLastError() << std::endl << std::flush;
		FILE_LOG(logERROR) << str << WSAGetLastError();
	}
}


Server::~Server()
{
	closesocket(s);
	WSACleanup();
}

void Server::run()
{
	std::map<char, std::function<void()>> ops =
	{
		{ 1, [this]() { sendFile(); } },
		{ 2, [this]() { recvFile(); } }
	};

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

		while (connected)
		{
			recvPacketWithACK(p);
			char choice = p.data[0];

			// Execute the choice
			auto result = ops.find(choice);
			if (result != ops.end())
			{
				result->second();
			}
			else
			{
				connected = false;
			}
		}
	}
	catch (const char* str)
	{
		std::cout << str << WSAGetLastError() << std::endl << std::flush;
		FILE_LOG(logERROR) << str << WSAGetLastError();
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
	FILE_LOG(logDEBUG) << "Received SYN: " << p.seqNo;

	expectedSeqNo = (~p.seqNo) & 1;

	p.ackNo = p.seqNo + 1;
	int synNum = rand();
	p.seqNo = synNum;
	p.syn = true;

	bool sent = false;

	while (!sent)
	{
		FILE_LOG(logDEBUG) << "Sending SYN+ACK: SYN -> " << p.seqNo << " ACK -> " << p.ackNo;
		sendPacket(p);

		FD_ZERO(&readfds);
		FD_SET(s, &readfds);

		if (select(1, &readfds, nullptr, nullptr, &tp) == 0)
		{
			FILE_LOG(logDEBUG) << "Timeout during handshake waiting for final ACK";
		}
		else
		{
			currentSeqNo = (~p.seqNo) & 1;
			sent = true;
			recvPacket(p);
			connectionAckNo = synNum + 1;

			FILE_LOG(logDEBUG) << "Received final ACK: " << p.ackNo << ", expected: " << connectionAckNo;

			if (p.ackNo != connectionAckNo)
				throw "Unexpected AckNo. Three way handshake failed\n";

			FILE_LOG(logDEBUG) << "Handshake success, connected to client!";
			std::cout << "Handshake success, connected to client!" << std::endl << std::flush;
			FILE_LOG(logDEBUG) << "Current SeqNo: " << currentSeqNo;
			FILE_LOG(logDEBUG) << "Expected SeqNo: " << expectedSeqNo;
		}
	}
}

void Server::sendFile()
{
	Packet p;

	// Get filename
	while (!recvPacketWithACK(p));

	std::string filename(p.data);

	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	size_t filesize = static_cast<size_t>(file.tellg());
	file.seekg(0);

	FILE_LOG(logDEBUG) << "Sending file " << filename << " with size " << filesize << " bytes";

	sendFile(file, filesize);
}

void Server::sendFile(std::ifstream& file, size_t filesize)
{
	size_t numPacketsSent{ 0 };

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
		numPacketsSent++;
	}

	std::cout << "File transfer complete" << std::endl;
	FILE_LOG(logDEBUG) << "File transfer complete";
	FILE_LOG(logDEBUG) << "\t# effective bytes sent: " << filesize;
	FILE_LOG(logDEBUG) << "\t# packets sent:         " << numPacketsSent;
	FILE_LOG(logDEBUG) << "\t# bytes sent:           " << numPacketsSent * sizeof(p);
}

void Server::recvFile()
{
	size_t numPacketsRecvd{ 0 };

	Packet p;

	// Get filename
	while (!recvPacketWithACK(p));

	std::string filename(p.data);

	std::ofstream file(filename, std::ios::binary | std::ios::trunc);

	FILE_LOG(logDEBUG) << "Receiving file " << filename;

	do
	{
		while (!recvPacketWithACK(p));
		numPacketsRecvd++;
		file.write(p.data, p.length);
	} while (p.length == Packet::DATA_LENGTH);

	file.close();

	std::ifstream newFile(filename, std::ios::ate);
	size_t filesize = static_cast<size_t>(newFile.tellg());

	std::cout << "File transfer complete" << std::endl;
	FILE_LOG(logDEBUG) << "File transfer complete";
	FILE_LOG(logDEBUG) << "\t# effective bytes sent: " << filesize;
	FILE_LOG(logDEBUG) << "\t# packets sent:         " << numPacketsRecvd;
	FILE_LOG(logDEBUG) << "\t# bytes sent:           " << numPacketsRecvd * sizeof(p);
}

void Server::sendPacketWithACK(const Packet& p)
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
		// Set which sockets to watch
		FD_ZERO(&readfds);
		FD_SET(s, &readfds);

		// TODO: Optimize so that it doesn't recopy to the buffer on a dropped packet
		sendPacket(p);
		FILE_LOG(logDEBUG) << "Sent packet " << packetCount << ", SeqNo " << p.seqNo;
		packetCount++;

		if (select(1, &readfds, nullptr, nullptr, &tp) == 0)
		{
			FILE_LOG(logDEBUG) << "Timeout while waiting for response. Resending...";
		}
		else
		{
			recvPacket(response);
			FILE_LOG(logDEBUG) << "Received response. AckNo = " << response.ackNo << ", expected = " << expectedAckNo;
			if (response.ackNo == expectedAckNo)
			{
				sent = true;
				currentSeqNo ^= 1;
				FILE_LOG(logDEBUG) << "Received proper ACK for packet " << packetCount - 1;
			}
			else
			{
				FILE_LOG(logDEBUG) << "Response AckNo not equal. Resending...";
			}
		}
	}
}

bool Server::recvPacketWithACK(Packet& p)
{
	Packet response;

	recvPacket(p);
	FILE_LOG(logDEBUG) << "Received packet, SeqNo = " << p.seqNo;

	if (p.seqNo == expectedSeqNo)
	{
		response.ackNo = p.seqNo;
		sendPacket(response);
		FILE_LOG(logDEBUG) << "Sending response, AckNo = " << response.ackNo;

		expectedSeqNo ^= 1;

		return true;
	}
	else
	{
		FILE_LOG(logDEBUG) << "Unexpected SeqNo " << p.seqNo << ", sending ACK";
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
	// Set to true for logging
	Server s(true);
	s.run();
	return 0;
}