// rUDP_sender.cpp

#include <winsock.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <conio.h>
#include "rUDP_sender.h"

#pragma comment(lib,"wsock32.lib")



DWORD __stdcall startMethodInThread(LPVOID arg);

rUDP_sender::rUDP_sender(void)
{
	// initialize
	nextseqnum = 1;
	sendbase = 1;
	N = 50;
	global_msg.content = "";
	global_msg.len = 0;
	NewMsgFlag = false;
	refuseFlag = 0;

	// register socket
	// every time instantiate the class, get one socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	DWORD dwThreadId;
	HANDLE hThread;
	hThread = CreateThread(
		NULL,                        // no security attributes 
		0,                           // use default stack size  
		startMethodInThread,                  // thread function 
		this,                // argument to thread function 
		0,                           // use default creation flags 
		&dwThreadId);                // returns the thread identifier
}

rUDP_sender::~rUDP_sender(void)
{

}

// close socket
int rUDP_sender::cancelsocket(void)
{
	closesocket(sock);
	sock = 0;
	return 0;
}

int rUDP_sender::bindsocket(sockaddr* addr, int size)
{
	bind(sock, addr, size);
	return 0;
}

// send function, multi-thread not allowed, because of using global flag
int rUDP_sender::sendMsg(char* message, int msg_len)
{
	int ret;
	NewMsgFlag = true; // set flag
	global_msg.content = message;
	global_msg.len = msg_len;
	while (refuseFlag == 0);
	if (refuseFlag == 1)
	{
		ret = 0;
	}
	else if (refuseFlag == 2)
	{

		ret = -1;
	}
	else
	{
		ret = -1;
	}
	refuseFlag = 0; // reset refuseFlag
	return ret;
}

int rUDP_sender::registerRemoteAddr(sockaddr* r_addr, int addr_size)
{
	remote.remote_addr = *r_addr;
	remote.size = addr_size;
	return 0;
}

int rUDP_sender::make_seg(Segment* seg, int nextseqnum, int nextacknum, char* data, int checksum)
{
	seg->seqnum = nextseqnum;
	seg->acknum = nextacknum;
	seg->length = strlen(data);// length of data
	seg->checksum = checksum;
	seg->payload = data;
	return 0;
}


int rUDP_sender::sendSeg(Segment seg)
{
	int retval;
	retval = sendto(sock, (char*)&seg, sizeof(seg), 0, (sockaddr*)&remote.remote_addr, remote.size);
	return 0;

}

int rUDP_sender::recvSeg(Segment* seg)
{
	char recvbuf[1024];
	int len, retval;
	len = recvfrom(sock, recvbuf, sizeof(recvbuf), 0, &remote.remote_addr, &remote.size);
	if (len <= 0)
	{
		return -1;
	}
	else
	{
		recvbuf[len] = 0;
		seg = (Segment*)recvbuf;
		return 0;
	}
}

DWORD WINAPI rUDP_sender::sender_proc(rUDP_sender* class_ptr)
{
	SOCKET s = class_ptr->sock;
	bool timeout = false;
	// select
	int retval, len;
	fd_set readfds;
	unsigned long mode = 1;// set nonblocking mode
	const timeval time{ 0, 100000 };
	FD_ZERO(&readfds);
	retval = ioctlsocket(sock, FIONBIO, &mode);
	if (retval == SOCKET_ERROR)
	{
		retval = WSAGetLastError();
		return -1;
	}
	cout << "select error !" << endl;


	int check_sum;
	char recvbuf[1024];
	Segment* recv_seg = new Segment;

	while (1)
	{
		// send msg event
		if (NewMsgFlag == true)
		{
			NewMsgFlag = false;
			if (nextseqnum < sendbase + N)
			{
				Segment seg;
				
				// get checksum
				check_sum = class_ptr->getchecksum(nextseqnum, 0, global_msg.content, global_msg.len);

				// send buffer to be done £¡£¡£¡
				global_msg.content[global_msg.len] = 0;// add terminator
				class_ptr->make_seg(&seg, nextseqnum, 0, global_msg.content, check_sum);// make segment
				class_ptr->sendSeg(seg);// send segment
				if (sendbase == nextseqnum)
					class_ptr->reset_timer();
				nextseqnum += seg.length;
				refuse_data(false);// accept
			}
			else
				refuse_data(true);// refuse
		}

		// select
		FD_SET(sock, &readfds);
		retval = select(0, &readfds, NULL, NULL, &time);// noblocking
		if (retval == SOCKET_ERROR)
		{
			retval = WSAGetLastError();
			break;
		}
		if (retval = FD_ISSET(sock, &readfds))
		{
			// recv event
			if (class_ptr->recvSeg(recv_seg) != -1)
			{
				check_sum = class_ptr->getchecksum(recv_seg->seqnum, recv_seg->acknum, recv_seg->payload, recv_seg->length);
				if (recv_seg->checksum == check_sum) // noncorrupt
				{
					sendbase = recv_seg->acknum;
					if (sendbase < nextseqnum)
						class_ptr->reset_timer();

				}
				else // corrupt
				{
					// do nothing
				}
			}
			else
			{
				// do nothing
			}


		}
		
		// timeout event
		if (timeout == true)
		{


		}
		



	}
}

int rUDP_sender::refuse_data(bool flag)
{
	if (flag == true)
		refuseFlag = 2;
	else
		refuseFlag = 1;
	return 0;
}

// to be done !!!
// reset timer
int rUDP_sender::reset_timer(void)
{
	// to be done
	return 0;
}

// to be done !!!
// get checksum
int rUDP_sender::getchecksum(int seqnum, int acknum, char* payload, int length)
{
	// to be done
	return 0;
}

DWORD __stdcall startMethodInThread(LPVOID arg)
{
	if (!arg)
	{
		return -1;
	}
	rUDP_sender* class_ptr = (rUDP_sender*)arg;
	class_ptr->sender_proc(class_ptr);
	return 0;
}

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
	remote_addr.sin_port = htons(8000);
	senddf.registerRemoteAddr((sockaddr*)&remote_addr, sizeof(remote_addr));
	// send a msg
	char *msg = "test";
	senddf.sendMsg(msg, strlen(msg));


	senddf.cancelsocket();
	WSACleanup();
	return 0;
}