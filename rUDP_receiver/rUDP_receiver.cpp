// rUDP_receiver.cpp

#include <winsock.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <conio.h>
#include "rUDP_receiver.h"

#pragma comment(lib,"wsock32.lib")


rUDP_receiver::rUDP_receiver(void)
{
	// initialize
	nextacknum = 1;

	// register socket
	// every time instantiate the class, get one socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
}

rUDP_receiver::~rUDP_receiver(void)
{

}

// close socket
int rUDP_receiver::cancelsocket(void)
{
	closesocket(sock);
	sock = 0;
	return 0;
}

int rUDP_receiver::bind_socket(sockaddr* addr, int size)
{
	bind(sock, addr, size);
	return 0;
}

int rUDP_receiver::recvMsg(char* seg)
{
	return 0;

}


int rUDP_receiver::sendSeg(SOCKET s, string sendseg)
{
	return 0;

}

int rUDP_receiver::recvSeg(SOCKET s, char* recvseg)
{
	return 0;

}


int main(int argc, char**argv)
{
	WSAData wsa;
	WSAStartup(0x101, &wsa);
	rUDP_receiver recvdf;
	
	// bind
	sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(8000);
	recvdf.bind_socket((sockaddr*)&local_addr, sizeof(local_addr));
	
	
	
	
	
	recvdf.cancelsocket();
	WSACleanup();

	return 0;
}