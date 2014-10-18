#include <winsock.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include "rUDP_sender.h"
#include <fstream>

#pragma comment(lib,"wsock32.lib")
using namespace std;

int main(int argc, char**argv)
{
	WSAData wsa;
	WSAStartup(0x101, &wsa);
	rUDP_sender senddf;
	sockaddr_in remote_addr;

	// register remote addr
	char* remote_ip = "127.0.0.1";
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_addr.s_addr = inet_addr(remote_ip);
	remote_addr.sin_port = htons(8016);
	senddf.registerRemoteAddr((sockaddr*)&remote_addr, sizeof(remote_addr));
	// send a msg
	char* msg;
	string s;
	int send_flag = 0;
	int numofsent = 0;
	bool first_time = true;

	ifstream fin("../alice.txt");
	if (!fin)
	{
		cout << "Error opening " << endl;
		system("pause");
		exit(-1);
	}
	while (getline(fin, s))
	{
		numofsent = 0;
		first_time = true;
		msg = const_cast<char*>(s.c_str());
		send_flag = senddf.sendMsg(msg, strlen(msg));
		numofsent = send_flag;
		while (strlen(msg) - numofsent != 0) // not sent all || error
		{
			if (send_flag != -1) // not sent all
			{
				numofsent += send_flag;
				send_flag = senddf.sendMsg(msg + numofsent, strlen(msg) - numofsent);
				first_time = false;
				continue;
			}

			while (send_flag == -1) // error
			{
				if (first_time == true)
				{
					send_flag = senddf.sendMsg(msg, strlen(msg));
					numofsent = 0;
				}
				else
				{
					send_flag = senddf.sendMsg(msg + numofsent, strlen(msg) - numofsent);
				}
				Sleep(10); // miliseconds
			}
			first_time = false;
		}
	}
	
	while (1);
	senddf.cancelsocket();
	WSACleanup();
	return 0;
}