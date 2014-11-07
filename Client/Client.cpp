#include "Client.h"

#include <iostream>
#include <map>
#include <functional>

#include "../Router/Router.h"
#include "../Utils/Utils.h"
#include "../Utils/log.h"

Client::Client(bool debug) :
enableLogging{ debug },
packetCount{ 0 }
{
	std::cout << "=== CLIENT ===" << std::endl << std::flush;
	try
	{
		// Initialize WinSock
		WSADATA wsadata;

		if (WSAStartup(0x0202, &wsadata) != 0)
			throw "Error in starting WSAStartup()\n";

		if (enableLogging)
		{
			FILELog::ReportingLevel() = logDEBUG;
			FILE *log_fd = fopen("client_log.txt", "w");
			Output2FILE::Stream() = log_fd;
		}
		else
		{
			FILELog::ReportingLevel() = logINFO;
		}

		char localhost[256];
		gethostname(localhost, 256);
		local_hostname = localhost;
		FILE_LOG(logDEBUG) << "Client started on " << local_hostname;

		// Create socket
		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == SOCKET_ERROR)
			throw "Socket creation failed\n";

		//Fill-in UDP Port and Address info.
		SOCKADDR_IN sa_in;
		sa_in.sin_family = AF_INET;
		sa_in.sin_port = htons(PEER_PORT_CLIENT);
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
		sa_out.sin_port = htons(ROUTER_PORT_CLIENT);

		srand((unsigned)time(nullptr));

		std::cout << "Client ready" << std::endl;
		FILE_LOG(logDEBUG) << "Client ready";
	}
	catch (const char *str)
	{
		std::cout << str << WSAGetLastError() << std::endl << std::flush;
		FILE_LOG(logERROR) << str << WSAGetLastError();
	}
}


Client::~Client()
{
	closesocket(s);
	WSACleanup();
}

void Client::run()
{
	std::map<char, std::function<void()>> ops = 
	{
		{ 1, [this]() { recvFile(); } },
		{ 2, [this]() { sendFile(); } }
	};

	Packet p;

	try
	{
		// Keep trying to connect to server
		threeWayHandshake();

		bool stayConnected = true;
		while (stayConnected)
		{
			char choice = selectOperation();

			// Send the choice
			p.seqNo = currentSeqNo;
			p.data[0] = choice;
			sendPacketWithACK(p);

			// Execute the choice
			auto result = ops.find(choice);
			if (result != ops.end())
			{
				result->second();
			}
			else
			{
				stayConnected = false;
			}
		}
	}
	catch (const char* str)
	{
		std::cout << str << WSAGetLastError() << std::endl << std::flush;
		FILE_LOG(logERROR) << str << WSAGetLastError();
	}
}

char Client::selectOperation()
{
	char choice;
	while (true)
	{
		std::cout << "Possible operations: " << std::endl
			<< " 1. GET" << std::endl
			<< " 2. PUT" << std::endl
			<< " 0. EXIT" << std::endl;
		std::cin >> choice;

		choice -= '0';

		if (0 <= choice && choice <= 2)
		{
			return choice;
		}
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

		FILE_LOG(logDEBUG) << "Sending SYN " << p.seqNo;
		sendPacket(p);

		FD_ZERO(&readfds);
		FD_SET(s, &readfds);

		if (select(1, &readfds, nullptr, nullptr, &tp) == 0)
		{
			FILE_LOG(logDEBUG) << "Timeout during handshake waiting for SYN+ACK";
		}
		else
		{
			currentSeqNo = (~p.seqNo) & 1;
			recvPacket(p);

			if (p.ackNo == expectedAckNo)
			{
				sent = true;
				expectedSeqNo = (~p.seqNo) & 1;
				FILE_LOG(logDEBUG) << "Received SYN+ACK: SYN -> " << p.seqNo << " ACK -> " << p.ackNo;
			}
			else
			{
				FILE_LOG(logDEBUG) << "Received SYN+ACK, unexpected AckNo " << p.ackNo << ", expected " << expectedAckNo;
			}
		}
	}

	p.ackNo = p.seqNo + 1;

	FILE_LOG(logDEBUG) << "Sending final ACK " << p.ackNo;
	connectionAck = p.ackNo;
	sendPacket(p);
	FILE_LOG(logDEBUG) << "Handshake success, connected to server!";
	std::cout << "Handshake success, connected to server!" << std::endl << std::flush;
	FILE_LOG(logDEBUG) << "Current SeqNo: " << currentSeqNo;
	FILE_LOG(logDEBUG) << "Expected SeqNo: " << expectedSeqNo;
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

	FILE_LOG(logDEBUG) << "Sending file " << filename << " with size " << filesize << " bytes";

	Packet p;
	p.seqNo = currentSeqNo;
	sprintf_s(p.data, filename.length() + 1, filename.c_str());
	sendPacketWithACK(p);

	sendFile(file, filesize);
}

void Client::sendFile(std::ifstream& file, size_t filesize)
{
	// Number of packets sent for this transfer, NOT total sent.
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
	size_t numPacketsRecvd{ 0 };

	Packet p;

	std::cout << "Select remote file to GET: ";
	std::string filename;
	std::cin >> filename;

	FILE_LOG(logDEBUG) << "Receiving file " << filename;

	p.seqNo = currentSeqNo;
	sprintf_s(p.data, filename.length() + 1, filename.c_str());
	sendPacketWithACK(p);

	std::ofstream file(filename, std::ios::binary | std::ios::trunc);

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

bool Client::recvPacketWithACK(Packet& p)
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
	// Set to true for logging
	Client c(true);
	c.run();
	return 0;
}