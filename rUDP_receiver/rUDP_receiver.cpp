// rUDP_receiver.cpp

#include <winsock.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "rUDP_receiver.h"
//#include "../checksum.h"

#pragma comment(lib,"wsock32.lib")
using namespace std;

#define TEST 1 // 0 - normal, 1 - test
#define prob 5

/////////////////////////
//----- Type defines ----------------------------------------------------------
typedef unsigned char      byte;    // Byte is a char
typedef unsigned short int word16;  // 16-bit word is a short int
typedef unsigned int       word32;  // 32-bit word is an int

//----- Defines ---------------------------------------------------------------
#define BUFFER_LEN        4096      // Length of buffer

//----- Prototypes ------------------------------------------------------------
word16 checksum(byte *addr, word32 count);


//=============================================================================
//=  Compute Internet Checksum for count bytes beginning at location addr     =
//=============================================================================
word16 checksum(byte *addr, word32 count)
{
	register word32 sum = 0;
	// Main summing loop
	int i = 0;
	while (count > 1)
	{
		sum = sum + *(((word16 *)addr) + i);
		i++;
		count = count - 2;
	}

	// Add left-over byte, if any
	if (count > 0)
		sum = sum + *((byte *)addr);

	// Fold 32-bit sum to 16 bits
	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return(~sum);
}

//////////////////////////////////////////




DWORD __stdcall startMethodInThread(LPVOID arg);

rUDP_receiver::rUDP_receiver(void)
{
	// initialize
	nextacknum = 1;
	newMsgFlag = false;
	memset(deliver_buffer, 0, sizeof(deliver_buffer));
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
	int retval;
	retval = bind(sock, addr, size);
	if (retval == SOCKET_ERROR)
	{
		retval = WSAGetLastError();
		return -1;
	}
	return 0;
}

int rUDP_receiver::recvMsg(char* msg)
{
	if (newMsgFlag == true)
	{
		newMsgFlag = false;
		memcpy(msg, deliver_buffer, strlen(deliver_buffer));
		msg[strlen(deliver_buffer)] = 0;
		//delete[] deliver_buffer; // ???
		memset(deliver_buffer, 0, sizeof(deliver_buffer));
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
	len = recvfrom(sock, recvbuf, sizeof(recvbuf), 0, &(remote.remote_addr), &(remote.size));
	if (len <= 0)
	{
		retval = WSAGetLastError();
		if (retval != WSAEWOULDBLOCK && retval != WSAECONNRESET)
		{
			cerr << "recv() failed !" << endl;
			return -1;
		}
		return-1;
	}
	else
	{
		recvbuf[len] = 0;
		memcpy(seg, recvbuf, len);
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
	const timeval blocking_time{ 0, 100000 };
	FD_ZERO(&readfds);
	retval = ioctlsocket(sock, FIONBIO, &mode);
	if (retval == SOCKET_ERROR)
	{
		retval = WSAGetLastError();
		return -1;
	}

	int check_sum;
	Segment* recv_seg = new Segment;
	Segment seg;

	while (1)
	{
		// select
		FD_SET(sock, &readfds);
		retval = select(0, &readfds, NULL, NULL, &blocking_time);// noblocking
		if (retval == SOCKET_ERROR)
		{
			retval = WSAGetLastError();
			break;
		}
		if (FD_ISSET(sock, &readfds))
		{
			// recv event
			if (class_ptr->recvSeg(recv_seg) != -1)
			{
				
				//-----------------------------TEST------------------------------------
				#if TEST == 1
				srand((unsigned)time(NULL));
				retval = (rand() % 100);
				if (retval < prob)
				{
					// intentionally ignore (i.e., drop) it with a certain probability
					continue;
				}
				#endif
				//----------------------------------------------------------------------
				
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


// get checksum
int rUDP_receiver::getchecksum(int seqnum, int acknum, char* payload, int length)
{
	// borrow from the Internet: http://www.csee.usf.edu/~christen/tools/checksum.c

	byte* str = new byte[sizeof(seqnum) + sizeof(acknum) + sizeof(payload) + sizeof(length)];
	memcpy(str, (byte*)&seqnum, sizeof(seqnum));
	memcpy(str + sizeof(seqnum), (byte*)&acknum, sizeof(acknum));
	memcpy(str + sizeof(seqnum) + sizeof(acknum), (byte*)payload, sizeof(payload));
	memcpy(str + sizeof(seqnum) + sizeof(acknum) + sizeof(payload), (byte*)&length, sizeof(length));

	word16 check = checksum(str, (word32)sizeof(str));
	delete [] str;
	return (int)check;
}

// buffer segment
int rUDP_receiver::buffer_seg(Segment* recv_seg)
{
	int retval;
	RecvNode recv_node;
	recv_node.seqnum = recv_seg->seqnum;
	recv_node.payload_len = recv_seg->length;
	memcpy(recv_node.seg, (char*)recv_seg, sizeof(Segment));
	if (recv_buf.index >= 10)
	{
		// memory substitution (replace the last element with the new one)
		recv_buf.index -= 1;
		retval = -1;
	}
	else
	{
		retval = 0;
	}
	insert_inorder(&recv_buf, recv_node); // insert in order
	return retval;
}

// deliver segment (set status)
int rUDP_receiver::deliver_seg(void)
{
	int len = 0, i, j, count = -1, offset;
	int* len_array = new int[recv_buf.index];
	for (j = 0; j < recv_buf.index; j++)
	{
		if (recv_buf.recv_array[j].seqnum == nextacknum)
		{
			break;
		}
	}

	if (j < recv_buf.index)
	{
		count = j;
		len += recv_buf.recv_array[j].payload_len;
		len_array[0] = recv_buf.recv_array[j].payload_len;
		for (i = j; i < recv_buf.index; i++)
		{
			if (recv_buf.recv_array[i].seqnum + recv_buf.recv_array[i].payload_len \
				== recv_buf.recv_array[i + 1].seqnum)
			{
				len += recv_buf.recv_array[i+1].payload_len;
				len_array[i-j+1] = recv_buf.recv_array[i+1].payload_len;
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
		//deliver_buffer = new char[len]; // copy data to deliver buffer
		for (i = j; i <= count; i++)
		{
			if (i == j) offset = 0;
			else offset = len_array[i - j - 1];
			memcpy(deliver_buffer + offset, ((Segment*)(recv_buf.recv_array[i].seg))->payload, ((Segment*)(recv_buf.recv_array[i].seg))->length);
		}
		deliver_buffer[len] = 0;
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
		recv_buf->index += 1;
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
