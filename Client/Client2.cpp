//// CLIENT TCP PROGRAM
//// Revised and tidied up by
//// J.W. Atwood
//// 1999 June 30
//
//
//
//char* getmessage(char *);
//
//
//
///* send and receive codes between client and server */
///* This is your basic WINSOCK shell */
//#pragma comment( linker, "/defaultlib:ws2_32.lib" )
//#include <winsock2.h>
//#include <ws2tcpip.h>
//
//#include <winsock.h>
//#include <stdio.h>
//#include <iostream>
//#include <string>
//#include <sstream>
//#include <fstream>
//#include <array>
//
//#include <windows.h>
//#include <dirent.h>
//#include "../Utils/Utils.h"
//#include "../Router/Router.h"
//
//using namespace std;
//
////user defined port number
//#define REQUEST_PORT 0x7000;
//
//const int port = 7000;
//
////socket data types
//SOCKET s;
//SOCKADDR_IN sa;         // filled by bind
//SOCKADDR_IN sa_in;      // fill with server info, IP, port
//
//const int BUFFER_SIZE = 512;
//const int MAX_FILENAME = 128;
//
////buffer data types
//char szbuffer[BUFFER_SIZE];
//
//char *buffer;
//
//int ibufferlen = 0;
//
//int ibytessent;
//int ibytesrecv = 0;
//
//
//
////host data types
//HOSTENT *hp;
//HOSTENT *rp;
//
//const int MAX_HOST_NAME = 128;
//
////char localhost[MAX_HOST_NAME],
////remotehost[MAX_HOST_NAME];
//
//HANDLE test;
//DWORD dwtest;
//
//
//void doGet();
//void doPut();
//void doList();
//
//int initializeWSADATA(WSADATA& wsadata)
//{
//	if (WSAStartup(0x0202, &wsadata) != 0)
//	{
//		throw "Error in starting WSAStartup()\n";
//	}
//
//	/* Display the wsadata structure */
//	cout << endl
//		<< "wsadata.wVersion " << wsadata.wVersion << endl
//		<< "wsadata.wHighVersion " << wsadata.wHighVersion << endl
//		<< "wsadata.szDescription " << wsadata.szDescription << endl
//		<< "wsadata.szSystemStatus " << wsadata.szSystemStatus << endl
//		<< "wsadata.iMaxSockets " << wsadata.iMaxSockets << endl
//		<< "wsadata.iMaxUdpDg " << wsadata.iMaxUdpDg << endl;
//
//	return 0;
//}
//
////int main()
////{
////	try
////	{
////		WSADATA wsadata;
////		initializeWSADATA(wsadata);
////
////		// Open the socket
////		SOCKET clientSock;
////		SOCKADDR_IN serverInfo;
////		HOSTENT *hp;
////		char localhostName[MAX_HOST_NAME];
////
////		// Retrieve the local hostname
////		gethostname(localhostName, MAX_HOST_NAME);
////
////		if ((hp = gethostbyname(localhostName)) == NULL)
////			throw "gethostbyname failed\n";
////
////		// Fill in server info
////		serverInfo.sin_family = AF_INET;
////		serverInfo.sin_port = htons(PEER_PORT_CLIENT);
////		serverInfo.sin_addr.s_addr = htonl(INADDR_ANY);
////
////		// Create the socket
////		if ((clientSock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
////			throw "Client socket creation failed\n";
////
////		// Bind to the client port
////		if (bind(clientSock, (LPSOCKADDR)&serverInfo, sizeof(serverInfo)) == SOCKET_ERROR)
////			throw "Client socket bind failed\n";
////
////		// Get router host name
////		char routerHostName[MAX_HOST_NAME];
////		getInput("Enter router host name: ", routerHostName);
////
////		//// Set up connection to the router
////		//SOCKADDR_IN sa;
////		//HOSTENT *hp;
////		//if ((hp = gethostbyname(hostname)) == NULL) throw "Could not determine a host address from supplied name";
////
////		//cout << "Peer connection: " << hostname << ":" << port << endl;
////
////		//// Fill in port and address information
////		//memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
////		//sa.sin_family = hp->h_addrtype;
////		//sa.sin_port = htons(port);
////	}
////	catch (const char *string)
////	{
////		std::cerr << string << WSAGetLastError() << std::endl << std::flush;
////	}
////}
//
////int main(void){
////
////    WSADATA wsadata;
////
////    try {
////
////		startWSA(wsadata);
////
////
////        //Display name of local host.
////
////        gethostname(localhost, 127);
////        cout << "Local host name is \"" << localhost << "\"" << endl;
////
////        hp = gethostbyname(localhost);
////
////        if (hp == nullptr)
////            throw "gethostbyname failed\n";
////
////        //Ask for name of remote server
////
////        cout << "please enter your remote server name: " << flush;
////        cin >> remotehost;
////        cout << "Remote host name is: \"" << remotehost << "\"" << endl;
////
////        rp = gethostbyname(remotehost);
////
////        if (rp == nullptr)
////            throw "remote gethostbyname failed\n";
////
////		//Create the Udp Sock1
////		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
////			throw "Create UDP Socket1 failed\n";
////
////		//Fill-in UDP Port and Address info.
////		sa_in.sin_family = AF_INET;
////		sa_in.sin_port = htons(port);
////		sa_in.sin_addr.s_addr = htonl(INADDR_ANY);
////
////		//Bind the UDP port1
////		if (bind(s, (LPSOCKADDR)&sa_in, sizeof(sa_in)) == SOCKET_ERROR)
////			throw "can't bind the socket1";
////
////		//creat peer host1
////		if ((hp = gethostbyname(peer_name1)) == NULL)
////			throw "get server name failed\n";
////		memset(&sa_in, 0, sizeof(sa_in));
////		memcpy(&sa_in.sin_addr, hp->h_addr, hp->h_length);
////		sa_in.sin_family = hp->h_addrtype;
////		sa_in.sin_port = htons(PEER_PORT1);
////
////
////        //sa_in.sin_family = rp->h_addrtype;
////        //sa_in.sin_port = htons(port);
////
////        //Display the host machine internet address
////
////        cout << "Connecting to remote host:";
////        cout << inet_ntoa(sa_in.sin_addr) << endl;
////
////        //Connect Client to the server
////        if (connect(s, (LPSOCKADDR)&sa_in, sizeof(sa_in)) == SOCKET_ERROR)
////            throw "connect failed\n";
////
////        /* Have an open connection, so, server is
////
////        - waiting for the client request message
////        - don't forget to append <carriage return>
////        - <line feed> characters after the send buffer to indicate end-of file */
////
////        //append client message to szbuffer + send.
////        bool quit = false;
////
////        while (!quit)
////        {
////            printf_s("\n\nSelect option:\n 1 - GET\n 2 - PUT\n 3 - LIST\n 4 - EXIT\n");
////            std::string input;
////            cin >> input;
////            char option = input[0] - '0';
////            szbuffer[0] = option;
////
////            ibytessent = 0;
////            ibufferlen = strlen(szbuffer);
////            ibytessent = send(s, szbuffer, 1, 0);
////            if (ibytessent == SOCKET_ERROR)
////                throw "Send failed\n";
////
////
////            switch (option)
////            {
////                case 1:
////                    doGet();
////                    break;
////                case 2:
////                    doPut();
////                    break;
////                case 3:
////                    doList();
////                    break;
////                case 4:
////                    quit = true;
////                    break;
////                default:
////                    printf_s("%d is not an option", option);
////            }
////        }
////
////    } // try loop
////
////    //Display any needed error response.
////
////    catch (char *str) { cerr << str << ":" << dec << WSAGetLastError() << endl; }
////
////    //close the client socket
////    closesocket(s);
////
////    /* When done uninstall winsock.dll (WSACleanup()) and exit */
////    WSACleanup();
////    return 0;
////}
//
//void doList()
//{
//    std::stringstream ss;
//
//    int size = getFileSize(s);
//
//    int numRecvs = (size / BUFFER_SIZE) + 1;
//	int remainder = size % BUFFER_SIZE;
//
//    for (int i = 0; i < numRecvs; i++)
//    {
//		int length = (i == numRecvs - 1) ? remainder : BUFFER_SIZE;
//
//        if (recv(s, szbuffer, length, 0) == SOCKET_ERROR)
//            throw "Receive remote directory failed\n";
//        szbuffer[length] = '\0';
//        ss << szbuffer;
//    }
//	cout << "\nDirectory Listing:\n";
//    cout << ss.str();
//}
//
//void doGet()
//{
//    doList();
//    char filename[MAX_FILENAME];
//    printf_s("Select the file to retrieve: ");
//    cin.ignore();
//    cin.getline(filename, MAX_FILENAME);
//
//    // Send the filename
//    if (send(s, filename, MAX_FILENAME, 0) == SOCKET_ERROR)
//        throw "Send filename failed\n";
//
//    // Receive whether the file exists or not?
//    if (recv(s, szbuffer, 1, 0) == SOCKET_ERROR)
//        throw "Error while receiving OK in doGet()\n";
//
//    bool fileExists = szbuffer[0] == 1;
//
//    if (fileExists)
//    {
//        int filesize = getFileSize(s);
//        printf_s("File size %d\n", filesize);
//
//		recvFile(s, filename, filesize, szbuffer, BUFFER_SIZE);
//
//        cout << "Done!\n";
//    }
//    else
//    {
//        cout << "That file does not appear to exist on the server\n";
//    }
//}
//
//void doPut()
//{
//	cout << "\nLocal Directory Listing:\n";
//	cout << getDirectoryItems();
//	printf_s("Please choose a file to put: ");
//	cin.ignore();
//	char filename[MAX_FILENAME];
//	cin.getline(filename, MAX_FILENAME);
//
//	ifstream file;
//	file.open(filename, ios::binary | ios::ate);
//
//	if (file.is_open())
//	{
//		int filenameLength = strlen(filename);
//
//		ibytessent = send(s, filename, MAX_FILENAME, 0);
//		if (ibytessent == SOCKET_ERROR)
//			throw "Send filename failed\n";
//
//		unsigned int ifilesize = (unsigned)file.tellg();
//		file.seekg(0);
//
//        sendFileSize(s, ifilesize);
//
//		int numBuffs = (ifilesize / BUFFER_SIZE) + 1;
//		int remainder = (ifilesize % BUFFER_SIZE);
//		
//		for (int i = 0; i < numBuffs; i++)
//		{
//			int length = i == numBuffs - 1 ? remainder : BUFFER_SIZE;
//
//			file.read(szbuffer, length);
//
//			ibytessent = send(s, szbuffer, length, 0);
//			if (ibytessent == SOCKET_ERROR)
//				throw "Send file failed\n";
//		}
//
//		printf_s("Done!\n");
//
//		file.close();
//	}
//	else
//	{
//		printf_s("File doesn't exist!\n");
//	}
//}
//
//
//
