//rUDP_receiver.h

using namespace std;

class rUDP_receiver // one to one & simplex version
{
public:
	rUDP_receiver(void);
	~rUDP_receiver();
	// Public API for Application
	int cancelsocket(void);
	int bind_socket(sockaddr* addr, int size);
	int recvMsg(char* msg);
	DWORD WINAPI receiver_proc(rUDP_receiver* class_ptr);


private:
	// Private Data & Function
	typedef struct{
		int seqnum;
		int acknum;
		int length;
		int checksum;
		char payload[128];
	}Segment;
	
	typedef struct{
		int seqnum;
		int payload_len;
		char seg[sizeof(Segment)];
	}RecvNode;

	typedef struct{
		int index = 0;
		RecvNode recv_array[10];
	}RecvBuf;

	RecvBuf recv_buf;

	// remote addr
	struct remoteAddr
	{
		sockaddr remote_addr;
		int size = sizeof(remote_addr);
	}remote;

	SOCKET sock;
	int nextacknum;
	bool newMsgFlag;
	char* deliver_buffer = new char[1024]; // to be improve !!

	// Function
	int make_seg(Segment* seg, int nextseqnum, int nextacknum, char* data, int dlen, int checksum);
	int sendSeg(Segment seg);
	int recvSeg(Segment* seg);
	int getchecksum(int seqnum, int acknum, char* payload, int length);
	int buffer_seg(Segment* recv_seg);
	int deliver_seg(void);
	int insert_inorder(RecvBuf* recv_buf, RecvNode recv_node);
	int delete_inorder(RecvBuf* recv_buf, int count);
};

