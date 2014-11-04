//#pragma once
//#pragma comment( linker, "/defaultlib:ws2_32.lib" )
//
//
//#include <winsock2.h>
//#include <ws2tcpip.h>
//#include <process.h>
//#include <winsock.h>
//#include <iostream>
//#include <windows.h>
//#include <fstream>
//#include "../Utils/Utils.h"
//
//
//using namespace std;
//
////port data types
//
//#define REQUEST_PORT 0x7070
//
//int port = REQUEST_PORT;
//
////socket data types
//SOCKET s;
//
//SOCKET s1;
//SOCKADDR_IN sa;      // filled by bind
//SOCKADDR_IN sa1;     // fill with server info, IP, port
//union {
//    struct sockaddr generic;
//    struct sockaddr_in ca_in;
//}ca;
//
//int calen = sizeof(ca);
//
//const int BUFFER_SIZE = 512;
//const int MAX_FILENAME = 128;
//
////buffer data types
//char szbuffer[BUFFER_SIZE];
//
//char *buffer;
//int ibufferlen;
//int ibytesrecv;
//
//int ibytessent;
//
////host data types
//char localhost[128];
//
//HOSTENT *hp;
//
////wait variables
//int nsa1;
//int r, infds = 1, outfds = 0;
//struct timeval timeout;
//const struct timeval *tp = &timeout;
//
//fd_set readfds;
//
////others
//HANDLE test;
//
//DWORD dwtest;
//
//void doGet();
//void doPut();
//void doList();
//
//int main123(void){
//
//    WSADATA wsadata;
//
//    try{
//        if (WSAStartup(0x0202, &wsadata) != 0)
//        {
//            cout << "Error in starting WSAStartup()\n";
//        }
//        else
//        {
//            buffer = "WSAStartup was suuccessful\n";
//            WriteFile(test, buffer, sizeof(buffer), &dwtest, NULL);
//
//            /* display the wsadata structure */
//            cout << endl
//                << "wsadata.wVersion " << wsadata.wVersion << endl
//                << "wsadata.wHighVersion " << wsadata.wHighVersion << endl
//                << "wsadata.szDescription " << wsadata.szDescription << endl
//                << "wsadata.szSystemStatus " << wsadata.szSystemStatus << endl
//                << "wsadata.iMaxSockets " << wsadata.iMaxSockets << endl
//                << "wsadata.iMaxUdpDg " << wsadata.iMaxUdpDg << endl;
//        }
//
//        //Display info of local host
//
//        gethostname(localhost, 127);
//        cout << "hostname: " << localhost << endl;
//
//        if ((hp = gethostbyname(localhost)) == NULL) {
//            cout << "gethostbyname() cannot get local host info?"
//                << WSAGetLastError() << endl;
//            exit(1);
//        }
//
//        //Create the server socket
//        if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
//            throw "can't initialize socket";
//        // For UDP protocol replace SOCK_STREAM with SOCK_DGRAM 
//
//
//        //Fill-in Server Port and Address info.
//        sa.sin_family = AF_INET;
//        sa.sin_port = htons(port);
//        sa.sin_addr.s_addr = htonl(INADDR_ANY);
//
//
//        //Bind the server port
//
//        if (bind(s, (LPSOCKADDR)&sa, sizeof(sa)) == SOCKET_ERROR)
//            throw "can't bind the socket";
//        cout << "Bind was successful" << endl;
//
//        //Successfull bind, now listen for client requests.
//
//        if (listen(s, 10) == SOCKET_ERROR)
//            throw "couldn't  set up listen on socket";
//        else cout << "Listen was successful" << endl;
//
//        FD_ZERO(&readfds);
//
//        //wait loop
//
//        while (1)
//
//        {
//
//            FD_SET(s, &readfds);  //always check the listener
//
//            if (!(outfds = select(infds, &readfds, NULL, NULL, tp))) {}
//
//            else if (outfds == SOCKET_ERROR) throw "failure in Select";
//
//            else if (FD_ISSET(s, &readfds))  cout << "got a connection request" << endl;
//
//            //Found a connection request, try to accept. 
//
//            if ((s1 = accept(s, &ca.generic, &calen)) == INVALID_SOCKET)
//                throw "Couldn't accept connection\n";
//
//            //Connection request accepted.
//            cout << "accepted connection from " << inet_ntoa(ca.ca_in.sin_addr) << ":"
//                << hex << htons(ca.ca_in.sin_port) << endl;
//
//            bool stayConnected = true;
//
//            while (stayConnected)
//            {
//                //Fill in szbuffer from accepted request.
//                if ((ibytesrecv = recv(s1, szbuffer, 1, 0)) == SOCKET_ERROR)
//                    throw "Receive error in server program\n";
//
//                char option = szbuffer[0];
//
//                switch (option) // Call appropriate function??
//                {
//                    case 1:
//                        cout << "Selected GET" << endl;
//                        doGet();
//                        break;
//                    case 2:
//                        cout << "Selected PUT" << endl;
//                        doPut();
//                        break;
//                    case 3:
//                        cout << "Selected LIST" << endl;
//                        doList();
//                        break;
//                    case 4:
//                        cout << "Disconnecting from " << inet_ntoa(ca.ca_in.sin_addr)
//                            << htons(ca.ca_in.sin_port) << endl;
//                        stayConnected = false;
//                        break;
//                    default:
//                        printf_s("%d is not an option.\n", option);
//                }
//            }
//
//        }//wait loop
//
//    } //try loop
//
//    //Display needed error message.
//
//    catch (char* str) { cerr << str << WSAGetLastError() << endl; }
//
//    //close Client socket
//    closesocket(s1);
//
//    //close server socket
//    closesocket(s);
//
//    /* When done uninstall winsock.dll (WSACleanup()) and exit */
//    WSACleanup();
//    return 0;
//}
//
//void doList()
//{
//    std::string directoryItems = getDirectoryItems();
//    unsigned int directoryStrSize = directoryItems.length();
//    const char *currentBuffer = directoryItems.c_str();
//
//    int numSends = (directoryStrSize / BUFFER_SIZE) + 1;
//	int remainder = directoryStrSize % BUFFER_SIZE;
//
//	sendFileSize(s1, directoryStrSize);
//
//    for (int i = 0; i < numSends; i++)
//    {
//		int length = (i == numSends - 1) ? remainder : BUFFER_SIZE;
//
//        ibytessent = send(s1, currentBuffer, length, 0);
//        if (ibytessent == SOCKET_ERROR)
//            throw "Send filename failed\n";
//        currentBuffer += length;
//    }
//}
//
//void doGet()
//{
//    doList();
//    std::cout << "Sending directory listing..." << std::endl;
//
//    char filename[MAX_FILENAME];
//
//    // Wait for filename
//    if ((ibytesrecv = recv(s1, filename, MAX_FILENAME, 0)) == SOCKET_ERROR)
//        throw "Receive error in server program\n";
//
//    printf_s("Trying to retrieve file: %s\n", filename);
//
//    if (fileExists(filename))
//    {
//        cout << "File exists!\n";
//
//        szbuffer[0] = 1;
//        if (send(s1, szbuffer, 1, 0) == SOCKET_ERROR)
//            throw "Error sending OK in doGet()\n";
//
//        ifstream file(filename, ios::binary | ios::ate);
//        unsigned int filesize = (unsigned int)file.tellg();
//        file.seekg(0);
//        sendFileSize(s1, filesize);
//
//        int numBuffs = (filesize / BUFFER_SIZE) + 1;
//        int remainder = (filesize % BUFFER_SIZE);
//
//        for (int i = 0; i < numBuffs; i++)
//        {
//            int length = i == numBuffs - 1 ? remainder : BUFFER_SIZE;
//
//            file.read(szbuffer, length);
//
//            ibytessent = send(s1, szbuffer, length, 0);
//            if (ibytessent == SOCKET_ERROR)
//                throw "Send file failed\n";
//        }
//        cout << "Done!\n";
//    }
//    else
//    {
//        cout << "File does NOT exist...\n";
//
//        szbuffer[0] = 0;
//        if (send(s1, szbuffer, 1, 0) == SOCKET_ERROR)
//            throw "Error sending NOT OK in doGet()\n";
//    }
//
//}
//
//void doPut()
//{
//	// TODO: If given an incorrect filename it screws up
//	char filename[MAX_FILENAME];
//
//	// Wait for filename
//	if ((ibytesrecv = recv(s1, filename, MAX_FILENAME, 0)) == SOCKET_ERROR)
//		throw "Receive error in server program\n";
//
//	printf_s("Receiving file: %s\n", filename);
//
//
//	std::string fname(filename);
//
//    while (fileExists(fname))
//	{
//		printf_s("File \"%s\" already exists! Renaming...\n", fname.c_str());
//		fname = appendCopy(fname);
//	}
//
//    ofstream file;
//    file.open(fname, ios::binary | ios::trunc);
//
//	int filesize = getFileSize(s1);
//
//	printf_s("File size: %d\n", filesize);
//
//	recvFile(s1, fname.c_str(), filesize, szbuffer, BUFFER_SIZE);
//
//	printf_s("Done!\n");
//}