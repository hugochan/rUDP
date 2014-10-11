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

DWORD __stdcall startMethodInThread(LPVOID arg);

rUDP_receiver::rUDP_receiver(void)
{
	// initialize
	nextacknum = 1;
	newMsgFlag = false;
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

int rUDP_receiver::recvMsg(char* msg)
{
	if (newMsgFlag == true)
	{
		newMsgFlag = false;
		memcpy(msg, deliver_buffer, sizeof(deliver_buffer)); //??
		delete deliver_buffer;
		return 0;
	}
	else
	{
		return -1;
	}

}

int rUDP_receiver::make_seg(Segment* seg, int nextseqnum, int nextacknum, char* data, int dlen, int checksum)
{
	seg->seqnum = nextseqnum;
	seg->acknum = nextacknum;
	seg->length = dlen;
	seg->checksum = checksum;
	memcpy(seg->payload, data, dlen);
	seg->payload[dlen] = 0;
	return 0;
}

int rUDP_receiver::sendSeg(Segment seg)
{
	int retval;
	retval = sendto(sock, (char*)&seg, sizeof(seg), 0, (sockaddr*)&remote.remote_addr, remote.size);
	return 0;

}

int rUDP_receiver::recvSeg(Segment* seg)
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

DWORD WINAPI rUDP_receiver::receiver_proc(rUDP_receiver* class_ptr)
{
	SOCKET s = class_ptr->sock;
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
	Segment* recv_seg = new Segment;
	Segment seg;

	while (1)
	{
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
					//buffer segment & deliver segment & update nextacknum
					class_ptr->buffer_seg(recv_seg);
					class_ptr->deliver_seg();
					
					// get checksum
					check_sum = class_ptr->getchecksum(0, nextacknum, "", 0);

					class_ptr->make_seg(&seg, 0, nextacknum, "", 0, check_sum);
					class_ptr->sendSeg(seg);

				}
				else // corrupt
				{
					// get checksum
					check_sum = class_ptr->getchecksum(0, nextacknum, "", 0);

					class_ptr->make_seg(&seg, 0, nextacknum, "", 0, check_sum);
					class_ptr->sendSeg(seg);
				}
			}
			else
			{
				// do nothing
			}
		}
	}
}


// to be done !!!
// get checksum
int rUDP_receiver::getchecksum(int seqnum, int acknum, char* payload, int length)
{
	// to be done
	return 0;
}

// buffer segment
int rUDP_receiver::buffer_seg(Segment* recv_seg)
{
	if (recv_buf.index < 10)
	{
		RecvNode recv_node;
		recv_node.seqnum = recv_seg->seqnum;
		recv_node.payload_len = recv_seg->length;
		memcpy(recv_node.seg, (char*)recv_seg, sizeof(Segment));
		insert_inorder(&recv_buf, recv_node); // insert in order
		return 0;
	}
	else
	{
		return -1; //discard 
	}
}

// deliver segment (set status)
int rUDP_receiver::deliver_seg(void)
{
	int len = 0, i, count = -1, offset;
	int* len_array = new int[recv_buf.index];
	if (recv_buf.recv_array[0].seqnum == nextacknum)
	{
		count = 0;
		len += recv_buf.recv_array[0].payload_len;
		len_array[0] = recv_buf.recv_array[0].payload_len;
		for (i = 0; i < recv_buf.index; i++)
		{
			if (recv_buf.recv_array[i].seqnum + recv_buf.recv_array[i].payload_len \
				== recv_buf.recv_array[i + 1].seqnum)
			{
				len += recv_buf.recv_array[i+1].payload_len;
				len_array[i+1] = recv_buf.recv_array[i+1].payload_len;
				count += 1;
			}
			else
			{
				break;
			}
		}
	}

	if (count >= 0)
	{
		deliver_buffer = new char[len]; // copy data to deliver buffer
		for (i = 0; i <= count; i++)
		{
			if (i == 0) offset = 0;
			else offset = len_array[i - 1];
			memcpy(deliver_buffer + offset, ((Segment*)(recv_buf.recv_array[i].seg))->payload, ((Segment*)(recv_buf.recv_array[i].seg))->length);
		}
		newMsgFlag = true;
		delete_inorder(&recv_buf, count); // free recv buffer
		nextacknum += len; // update nextacknum
		delete[] len_array;
		return 0;
	}
	else
	{
		delete [] len_array;
		return -1;
	}
}

int rUDP_receiver::insert_inorder(RecvBuf* recv_buf, RecvNode recv_node)
{
	int i, j;
	if (recv_buf->index >= 10) return -1;
	else
	{
		for (i = 0; i < recv_buf->index; i++)
		{
			if (recv_node.seqnum < recv_buf->recv_array[i].seqnum)
			{
				break;
			}
		}

		for (j = recv_buf->index; j > i; j--)
		{
			recv_buf->recv_array[j] = recv_buf->recv_array[j - 1];
		}
		recv_buf->recv_array[i] = recv_node;
		return 0;
	}

}

// delete in order
int rUDP_receiver::delete_inorder(RecvBuf* recv_buf, int count)
{
	int i;
	if (count + 1 > recv_buf->index)
	{
		return -1;
	}
	else
	{
		for (i = 0; i < recv_buf->index - count - 1; i++)
		{
			recv_buf->recv_array[i] = recv_buf->recv_array[i + count + 1];
		}
		recv_buf->index -= (count + 1);
		return 0;
	}
}


DWORD __stdcall startMethodInThread(LPVOID arg)
{
	if (!arg)
	{
		return -1;
	}
	rUDP_receiver* class_ptr = (rUDP_receiver*)arg;
	class_ptr->receiver_proc(class_ptr);
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
	char * msg = new char[2048];

	while (1)
	{
		if (recvdf.recvMsg(msg) == 0)
		{
			cout << "new msg" << endl;
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