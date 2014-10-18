=====================================================================
                                                                       
                              ======                                   
                              README                                   
                              ======                                   
                                                                       
                             rUDP 1.0
                             17 Oct. 2014
                             author:Hugo Chan
                          
                      C++ Programs for Socket Programming 
                 																			                                              
=====================================================================

Contents:
-----------
1. Description
2. Implementation
3. Bugs
4. Notice



------------------------
1. Description:
------------------------

Reliable end-to-end in-order data delivery inside the application layer on top of the UDP.


------------------------
2. Implementation:
------------------------
-Finite State Machine design for implementation of interactions between the sender and receiver.
-Buffer Design:
---rUDP_sender:
	typedef struct
	{
		char buf[MAXNUM];
		int front, rear;
	}QueueBuf;
	
	// Buffer Information Management 
	// linked queue
	typedef struct // element type
	{
		int seqnum = 0;
		int length = 0;
	}Info;
	
		typedef struct node
	{ 
		Info info;
		node* next = NULL;
	}InfoNodeType;

	typedef struct
	{
		InfoNodeType* front;
		InfoNodeType* rear;
	}BufInfoManagLq;
	
---rUDP_receiver:
	typedef struct{
		int seqnum;
		int payload_len;
		char seg[sizeof(Segment)];
	}RecvNode;
	
	typedef struct{
		int index = 0;
		RecvNode recv_array[10];
	}RecvBuf;

-multiple threads for FSM and main function


----------------------------
3. Bugs: 
----------------------------

the current version has something wrong with the test_sender.cpp file. Because sometimes it can
only send part of the single line read from the input file. But the rUDP_sender class is fine, we
can fix the issue in our application program in test_sender.cpp file.

----------------------------
4. Notice: 
----------------------------
When receive buffer is full, it automatically adopts memory substitution(replace the last element with the new one).
Also:
no trunc£¬
no fast retransmit£¬
retransmit one each time£¬
initial seqnum predefined£¬
the sender leaves the acknum empty in its sent segment£¬
the receiver leaves seqnum empty in its feedback segment£¬
no size-limitation and reuse for the sequence number...