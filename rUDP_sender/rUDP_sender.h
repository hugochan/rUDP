//rUDP_sender.h

using namespace std;

class rUDP_sender // one to one & simplex version 
{
public:
	SOCKET sock;
	rUDP_sender(void);
	~rUDP_sender();
	// Public API for Application
	int cancelsocket(void);
	int bindsocket(sockaddr* addr, int size);
	int sendMsg(char* message, int msg_len);
	int registerRemoteAddr(sockaddr* r_addr, int addr_size);
	DWORD WINAPI sender_proc(rUDP_sender* class_ptr);


private:
	// Private Data & Function
	typedef struct{
		int seqnum;
		int acknum;
		int length;
		int checksum;
		char payload[128];
	}Segment;

	int nextseqnum, sendbase;
	#define CWND (int)50
	struct msg
	{
		char* content;
		int len;
	}global_msg;

	// remote addr
	struct remoteAddr
	{
		sockaddr remote_addr;
		int size;
	}remote;
	
	// Sender Buffer
	// cirular queue
	#define MAXNUM (CWND + 1)
	typedef struct
	{
		char buf[MAXNUM];
		int front, rear;
	}QueueBuf;
	QueueBuf queue_buf;

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
	InfoNodeType info_node;

	typedef struct
	{
		InfoNodeType* front;
		InfoNodeType* rear;
	}BufInfoManagLq;
	BufInfoManagLq buf_info_manag;


	bool NewMsgFlag;
	int refuseFlag;// -2 - default, nonnegative num(num of sent) - accept, -1 - refuse


	// Function
	int make_seg(Segment* seg, int nextseqnum, int nextacknum, char* data, int dlen, int checksum);
	int sendSeg(Segment seg);
	int recvSeg(Segment* seg);
	int reset_timer(void);
	int getchecksum(int seqnum, int acknum, char* payload, int length);
	int refuse_data(int flag);
	int enlqueue(BufInfoManagLq* q, Info x);
	int delqueue(BufInfoManagLq* q, Info* x);
	int checklqueue(BufInfoManagLq* q, Info* x); // check first element
	int lookuplqueue(BufInfoManagLq* q, int cond_seqnum); // look up for the specific element
	int enqueue(QueueBuf* q, char x);
	int dequeue(QueueBuf* q, char* x);
	int checkqueue(QueueBuf* q, char* x); // check first element

};

