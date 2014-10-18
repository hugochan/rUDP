// rUDP_sender.cpp

#include <winsock.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "rUDP_sender.h"
#include "../checksum.h"


#pragma comment(lib,"wsock32.lib")
using namespace std;

#define TEST 0 // 0 - normal, 1 - test
#define prob 5
#define timeout 500

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

rUDP_sender::rUDP_sender(void)
{
	// initialize
	nextseqnum = 1;
	sendbase = 1;
	queue_buf.front = 0;
	queue_buf.rear = 0;

	// initialize Buffer Information Management
	buf_info_manag.front = &info_node;
	buf_info_manag.rear = &info_node;

	global_msg.content = "";
	global_msg.len = 0;
	NewMsgFlag = false;
	refuseFlag = -2;
	time_start = 0;

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

// send function, blocking & multi-thread not allowed, because of using global flag
// return num of msg sent
int rUDP_sender::sendMsg(char* message, int msg_len)
{
	int ret;
	NewMsgFlag = true; // set flag
	message[msg_len] = 0;
	global_msg.content = message;
	global_msg.len = msg_len;
	while (refuseFlag == -2);
	if (refuseFlag >= 0)
	{
		ret = refuseFlag;
	}
	else if (refuseFlag == -1)
	{

		ret = -1;
	}
	else
	{
		ret = -1;
	}
	refuseFlag = -2; // reset refuseFlag
	return ret;
}

int rUDP_sender::registerRemoteAddr(sockaddr* r_addr, int addr_size)
{
	remote.remote_addr = *r_addr;
	remote.size = addr_size;
	return 0;
}

int rUDP_sender::make_seg(Segment* seg, int nextseqnum, int nextacknum, char* data, int dlen, int checksum)
{
	seg->seqnum = nextseqnum;
	seg->acknum = nextacknum;
	seg->length = dlen;// length of data
	seg->checksum = checksum;
	memcpy(seg->payload, data, dlen);
	seg->payload[dlen] = 0;
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
		memcpy(seg, recvbuf, len);
		return 0;
	}
}

DWORD WINAPI rUDP_sender::sender_proc(rUDP_sender* class_ptr)
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


	int check_sum, i, j, index;
	Segment* recv_seg = new Segment;
	Segment seg;
	char resend_buf[1024];

	Info buf_info;

	while (1)
	{
		// send msg event
		if (NewMsgFlag == true)
		{
			NewMsgFlag = false;
			if (nextseqnum < sendbase + CWND)
			{
				// send the new message once allowed, no waiting

				// copy to send buffer (enqueue)
				for (i = 0; i < global_msg.len; i++)
				{
					if (class_ptr->enqueue(&queue_buf, global_msg.content[i]) == -1)
						break;
				}

				// mantain Buffer Information Management (enlqueue)
				buf_info.seqnum = nextseqnum;
				buf_info.length = i;
				class_ptr->enlqueue(&buf_info_manag, buf_info);

				// get checksum
				check_sum = class_ptr->getchecksum(nextseqnum, 0, global_msg.content, i);
				
				// make & send segment
				class_ptr->make_seg(&seg, nextseqnum, 0, global_msg.content, i, check_sum);// make segment
				class_ptr->sendSeg(seg);// send segment
				if (first_run == true)
				{
					class_ptr->reset_timer();
					first_run = false;
				}
				if (sendbase == nextseqnum)
				{
					class_ptr->reset_timer(); // reset timer
				}
				nextseqnum += i;
				class_ptr->refuse_data(i); // response to upper layer
			}
			else
				class_ptr->refuse_data(-1); // response to upper layer
		}

		// select
		FD_SET(sock, &readfds);
		retval = select(0, &readfds, NULL, NULL, &blocking_time);// noblocking
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
				
				//-----------------------------TEST------------------------------------
				#if TEST == 1
				srand((unsigned int)time(NULL));
				retval = (rand() % 100);
				if (retval < prob)
				{
					// intentionally assume the segment is corrupted with a certain probability
					while(recv_seg->checksum == check_sum)
					{
						check_sum += 1;
					}
				}
				#endif
				//---------------------------------------------------------------------

				if (recv_seg->checksum == check_sum) // noncorrupt
				{
					sendbase = recv_seg->acknum;
					if (sendbase <= nextseqnum)
						class_ptr->reset_timer(); // reset timer

					// mantain Buffer Information Management (delqueue)
					index = class_ptr->lookuplqueue(&buf_info_manag, recv_seg->acknum);
					for (j = 0; j < index + 1; j++)
					{
						if (class_ptr->delqueue(&buf_info_manag, &buf_info) == -1)
						{
							cout << "delqueue error!" << endl;
							system("pause");
							exit(0);
						}
					}

					// free send buffer (dequeue)
					queue_buf.front = (recv_seg->acknum - 1) % MAXNUM;

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
		if (clock() - time_start >= timeout)
		{
			if (class_ptr->checklqueue(&buf_info_manag, &buf_info) == -1)
			{
				cout << "buf_info_manag  error!"<< endl;
				system("pause");
				exit(0);
			}
			if (buf_info.seqnum == sendbase)
			{
				if ((sendbase % MAXNUM) + buf_info.length - 1 < MAXNUM)
				{
					memcpy(resend_buf, queue_buf.buf + (sendbase % MAXNUM), buf_info.length);
				}
				else
				{
					memcpy(resend_buf, queue_buf.buf + (sendbase % MAXNUM), MAXNUM - (sendbase % MAXNUM));
					memcpy(resend_buf, queue_buf.buf, buf_info.length - (MAXNUM - (sendbase % MAXNUM)));
				}
				
				// get checksum
				check_sum = class_ptr->getchecksum(sendbase, 0, resend_buf, buf_info.length);

				// make & send segment
				class_ptr->make_seg(&seg, sendbase, 0, resend_buf, buf_info.length, check_sum);// make segment
				class_ptr->sendSeg(seg);// send segment

				class_ptr->reset_timer(); // reset timer
			}
			else
			{
				cout << "buf_info_manag error!" << endl;
				system("pause");
				exit(0);
			}
		}
	}
}

int rUDP_sender::refuse_data(int flag)
{
	if (flag == -1) // refuse
		refuseFlag = -1;
	else if (flag >= 0) // num of sent
	{
		refuseFlag = flag;
	}
	else // treated as refuse
	{
		refuseFlag = -1;
	}
	return 0;
}

// reset timer
int rUDP_sender::reset_timer(void)
{
	time_start = clock();
	return 0;
}

// get checksum
int rUDP_sender::getchecksum(int seqnum, int acknum, char* payload, int length)
{
	// borrow from the Internet: http://www.csee.usf.edu/~christen/tools/checksum.c

	byte* str = new byte[sizeof(seqnum)+sizeof(acknum)+sizeof(payload)+sizeof(length)];
	memcpy(str, (byte*)&seqnum, sizeof(seqnum));
	memcpy(str + sizeof(seqnum), (byte*)&acknum, sizeof(acknum));
	memcpy(str + sizeof(seqnum)+sizeof(acknum), (byte*)payload, sizeof(payload));
	memcpy(str + sizeof(seqnum)+sizeof(acknum)+sizeof(payload), (byte*)&length, sizeof(length));

	word16 check = checksum(str, (word32)sizeof(str));
	delete[] str;
	return (int)check;
}


// enqueue for BufInfoManagLq
int rUDP_sender::enlqueue(BufInfoManagLq *q, Info x)
{
	InfoNodeType  *s = new InfoNodeType;
	if (s != NULL)
	{
		s->info = x;
		s->next = NULL;	 // generate new node
		q->rear->next = s;	// add new node in a queue
		q->rear = s;		 // update rear pointer
		return 0;
	} 
	else 
		return -1; 
}

// dequeue for BufInfoManagLq
int rUDP_sender::delqueue(BufInfoManagLq *q, Info* x)
{
	InfoNodeType  *p;
	if (q->front->next == NULL)
		return -1;
	else
	{
		p = q->front->next;   // get front node
		*x = p->info;			// get element
		q->front->next = p->next; // update front pointer
		if (p->next == NULL)    // if length of queue equals to one
			q->rear = q->front; // update rear pointer
		delete p;		       // free the node
		return 0;
	}
}

// check for BufInfoManagLq
int rUDP_sender::checklqueue(BufInfoManagLq* q, Info* x)
{
	InfoNodeType  *p;
	if (q->front->next == NULL)
		return -1;
	else
	{
		p = q->front->next;   // get front node
		*x = p->info;			// get element
		return 0;
	}
}


// // look up for the specific element, return index
int rUDP_sender::lookuplqueue(BufInfoManagLq* q, int cond_seqnum) // look up for the specific element
{
	int index = -1;
	InfoNodeType* x = q->front->next;
	while (x != NULL)
	{
		if (x->info.seqnum + x->info.length - 1 >= cond_seqnum)  
		{
			// in this design, must have x->info.seqnum + x->info.length = cond_seqnum
			break;
		}
		index += 1;
		x = x->next;
	}
	return index;
}

// enqueue for Sender Buffer
int rUDP_sender::enqueue(QueueBuf* q, char x)
{
	if ((q->rear + 1) % MAXNUM == q->front) // full
		return -1;
	else
	{ 
		q->rear = (q->rear + 1) % MAXNUM; // update rear
		q->buf[q->rear] = x;	   // add new element
		return 0; 
	}
}

// dequeue Sender Buffer
int rUDP_sender::dequeue(QueueBuf *q, char* x)
{
	if (q->rear == q->front) // empty queue
	{	 
		return -1;
	}
	else
	{
		q->front = (q->front + 1) % MAXNUM; // update rear
		*x = q->buf[q->front];
		return 0;
	}
}

int rUDP_sender::checkqueue(QueueBuf* q, char* x)
{
	if (q->rear == q->front) // empty queue
	{
		return -1;
	}
	else
	{
		*x = q->buf[(q->front + 1) % MAXNUM];
		return 0;
	}
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