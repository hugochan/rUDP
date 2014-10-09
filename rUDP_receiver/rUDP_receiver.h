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
	int recvMsg(char* seg);

private:
	// Private Data & Function
	struct segment{
		int seqnum;
		int acknum;
		int length;
		int checksum;
		char payload[128];
	}; 
	
	SOCKET sock;
	int nextacknum;

	// Function
	int sendSeg(SOCKET s, string sendseg);
	int recvSeg(SOCKET s, char* recvseg);

};

