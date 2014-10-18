#include <winsock.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "rUDP_receiver.h"

#pragma comment(lib,"wsock32.lib")
using namespace std;


int main(int argc, char**argv)
{
	WSAData wsa;
	WSAStartup(0x101, &wsa);
	rUDP_receiver recvdf;

	// bind
	sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(8016);
	recvdf.bind_socket((sockaddr*)&local_addr, sizeof(local_addr));
	char msg[2048];

	while (1)
	{
		if (recvdf.recvMsg(msg) == 0)
		{
			cout << msg << endl;
		}
		else
		{

		}
	}



	recvdf.cancelsocket();
	WSACleanup();

	return 0;
}