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
		char* payload;// alter
	}Segment;

	int nextseqnum, sendbase, N;
	struct msg
	{
		char* content;
		int len;
	}global_msg;

	struct remoteAddr
	{
		sockaddr remote_addr;
		int size;
	}remote;
	bool NewMsgFlag;
	int refuseFlag;// 0 - default, 1 - accept, 2 - refuse


	// Function
	int make_seg(Segment* seg, int nextseqnum, int nextacknum, char* data, int checksum);
	int sendSeg(Segment seg);
	int recvSeg(Segment* seg);
	int reset_timer(void);
	int getchecksum(int seqnum, int acknum, char* payload, int length);
	int refuse_data(bool flag);
};

