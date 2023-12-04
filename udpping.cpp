/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define MAXLINE 60000
#define WINDOW_SIZE 200
#define QUEUE_CAPACITY 1000
#define errquit(m) { perror(m); exit(-1); }
using namespace std;

char sendBuffer[MAXLINE];
char rcvBuffer[MAXLINE];
int curFile = 0;
int seq = 1;
int servSeq = -1;
int lastACK = -1;

class Packet{
public:
	int seq;
	int ack;
	bool fin;
	uint16_t cksum;
	int len; // only length of data
	string filename;
	bool fileEnd;
	unsigned char* data;

	bool isAcked;

	Packet(char* raw){
		// cout << "\t\tconstructor with string parameters\tseq: " << seq << "\n";
		string _raw = raw;
		// cout << raw << endl;
		stringstream ss; ss.clear();
		ss << raw;
		ss >> seq >> ack >> fin >> cksum;
		len = 0;
		fileEnd = 0;
		filename = "";
		data = nullptr;
		// cout << "*";
	}

	// copy constructor
	Packet(const Packet &pkt){
		// cout << "\t\tcopy constructor called\nseq: " << pkt.seq << "\n";
		this->seq = pkt.seq; this->ack = pkt.ack; this->cksum = pkt.cksum; this->len = pkt.len;
		this->filename = pkt.filename; this->fileEnd = pkt.fileEnd; this->fin = pkt.fin;
		if(pkt.data != nullptr){
			this->data = new unsigned char[pkt.len + 1];
			memmove(this->data, pkt.data, pkt.len);
			this->data[len] = '\0';
		} else {
			this->data = nullptr;
		}
	}

	Packet(int _seq, bool _fin, int _len, char* _filename, bool _fileEnd, char* _data){
		// cout << "\t\tconstructor with multiple parameters called\tseq: " << seq << "\n";
		seq = _seq; ack = 0; fin = _fin; len = _len;
		fileEnd = _fileEnd;
		if(_filename != nullptr){
			filename = _filename;
		} else {
			filename = "";
		}
		if(_data != nullptr){
			data = new unsigned char[_len + 1];
			memmove(data, _data, _len);
			data[_len] = '\0';
		} else {
			data = nullptr;
		}
		isAcked = false;
	};

	// move constructor
	Packet(Packet&& pkt){ // : data(nullptr)
		// cout << "\t\tmove constructor called\tseq: " << pkt.seq << "\n";
		data = pkt.data;
		pkt.data = nullptr;
	}

	// move assignment operator
	Packet& operator=(Packet&& other){
		// cout << "\t\tmove assignment operator called\tseq: " << other.seq << "\n";
		if(this != &other){
			// delete [] data;
			data = other.data;
			other.data = nullptr;
			len = other.len; seq = other.seq; ack = other.ack; fin = other.fin;
			fileEnd = other.fileEnd; filename = other.filename; cksum = other.cksum;
		}
		return *this;
	}

	Packet &operator=(const Packet &other){
		// cout << "\t\toverloaded assignment called\tassign seq " << other.seq << " to seq " << seq << "\n";
		if(this != &other){
			// delete [] data;
			data = new unsigned char[other.len + 1];
			memmove(data, other.data, len);
			data[len] = '\0';
		}
		return *this;
	}

	~Packet(){
		// cout << "\t\tdestructor called\tseq: " << seq << "\n";
		if(data != nullptr) delete[] data;
	}

	void calculateCksum(){
		unsigned short *ptr = (unsigned short*)data;
		cksum = ptr[0];
		int round = len / 2;
		for(int i = 1; i < round; i++){
			cksum = cksum ^ ptr[i];
		}
	}
	void print(){
		// printf("====================================================\n");
		// printf("seq: %d\nACK: %d\nfin: %d\ncksum: %hu\nfilename: %s\nContent Length: %d\nfileEnd: %d\n%s\n",
		// 	seq, ack, fin, cksum, filename.c_str(), len, fileEnd, data);
		// printf("====================================================\n");
		cout << " seq: " << seq << "\tack: " << ack << "\n";
	}
	void send(int sockfd){
		bzero(&sendBuffer, sizeof(sendBuffer));
		// sprintf(sendBuffer, "seq: %d\nACK: %d\nfin: %d\ncksum: %hu\nfilename: %s\nfileEnd: %d\n%s",
		// 	seq, ack, fin, cksum, filename.c_str(), fileEnd, data);
		sprintf(sendBuffer, "%d\n%d\n%d\n%hu\n%d\n%s\n%d\n%s",
			seq, ack, fin, cksum, len, filename.c_str(), fileEnd, data);
		int n;
		if((n = write(sockfd, sendBuffer, sizeof(sendBuffer))) < 0) errquit("client write");
		// cout << "sent\n";
	}
};

static int sockfd = -1;
static struct sockaddr_in sin;
vector<Packet> waitQueue;
int cnt = WINDOW_SIZE;
int lastSentSeq = -1;
int totalFile;
char* fileDir;

void send1File(int sockfd);
Packet rcv(int sockfd);

int main(int argc, char *argv[]) {
	
	if(argc < 3) {
		return -fprintf(stderr, "usage: %s ... <port> <ip>\n", argv[0]);
	}

	fileDir = argv[1];
	totalFile = atoi(argv[2]);

	srand(time(0) ^ getpid());

	// setvbuf(stdin, nullptr, _IONBF, 0);
	// setvbuf(stderr, nullptr, _IONBF, 0);
	// setvbuf(stdout, nullptr, _IONBF, 0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-2], nullptr, 0));
	if(inet_pton(AF_INET, argv[argc-1], &sin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[1]);
	}
	socklen_t len = sizeof(sin);

	if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){ errquit("client socket");}
	else cout << "successfully create socket\n";

	struct timeval timeout = {1, 0}; //set timeout for 1 seconds
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));

	if(connect(sockfd, (struct sockaddr *) &sin, sizeof(sin)) < 0){ errquit("client connect") } 
	else cout << "successfully connect to server\n";

	// hand shaking
	// Packet(int _seq, bool _fin, int _len, char* _filename, bool _fileEnd, char* _data)
	Packet handshaking = Packet(0, 1, 0, nullptr, 0, nullptr);
	handshaking.print();
	while(1){
		handshaking.send(sockfd);
		// bzero(&handshaking, sizeof(handshaking));
		
		bzero(&rcvBuffer, sizeof(rcvBuffer));
		int n;
		if((n = read(sockfd, rcvBuffer, sizeof(rcvBuffer))) >= 0){
			cout << "handshaking success\n";
			Packet tmp = Packet(rcvBuffer);
			cout << "rcv";
			tmp.print();
			if(tmp.ack > lastACK && tmp.ack < seq) lastACK = tmp.ack;
			break;
		} else {
			cout << "timeout resend\n";
		}
	}
	int n;
	// start transfer data
	while(1){
		// cout << "================================\n";
		// cout << "waitqueue.size: " << waitQueue.size() << "\n";
		// for (auto it = waitQueue.begin(); it < waitQueue.end(); it++)
		// 	cout << it->seq << " ";
		// cout << "\nseq: " << seq << " / lastACK: " << lastACK << "\n";

		cout << "send1File() ==========================================\n";
		send1File(sockfd);
		cout << "=======================================================\n";

		cout << "waitqueue.size: " << waitQueue.size() << "\n";
		for (auto it = waitQueue.begin(); it < waitQueue.end(); it++)
			cout << it->seq << " ";
		
		cout << "\nrcv ACK ===============================================\n";
		// rcv ACK
		while(1){
			bzero(&rcvBuffer, sizeof(rcvBuffer));
			if((n = read(sockfd, rcvBuffer, sizeof(rcvBuffer))) < 0){
				cout << "timeout\n";
				break;
			}
			Packet tmp(rcvBuffer);
			cout << "rcv";
			tmp.print();
			if(tmp.ack > lastACK && tmp.ack < seq) lastACK = tmp.ack;
		}
		cout << "========================================================\n";

		cout << "remove ACKed pkt from waitQueue ========================\n";
		while(1){
			if(waitQueue.empty()) break;
			auto it = waitQueue.begin();
			// cout << it->seq << "\n";
			if(it->seq <= lastACK) {
				// delete [] it->data;
				waitQueue.erase(it);
			}
			else break;
		}
		if(curFile == totalFile && waitQueue.empty()) break;
		// cout << "curFile: " << curFile << "\nwaitQueue.size:  " << waitQueue.size() << "\n";
		cout << "lastACK: " << lastACK << "\n";
		cout << "waitqueue.size: " << waitQueue.size() << "\n";
		for (auto it = waitQueue.begin(); it < waitQueue.end(); it++)
			cout << it->seq << " ";
		cout << "======================================================\n";
	}

	close(sockfd);
}

Packet rcv(int sockfd) {
	int n;
	bzero(&rcvBuffer, sizeof(rcvBuffer));
	if((n = read(sockfd, rcvBuffer, sizeof(rcvBuffer))) < 0) errquit("client read");
	return Packet(rcvBuffer);
}

void send1File(int sockfd){
	// send pkt in waitQueue
	for(auto it = waitQueue.begin(); it < waitQueue.end(); it++){
		it->print();
		// cout << it->data << "\n";
		it->send(sockfd);
	}
	
	if(waitQueue.size() >= QUEUE_CAPACITY) return;
	
	// send 10 files a time
	char buf[MAXLINE - 50];
	int cnt = 1, n;
	while(cnt-- && curFile < totalFile){
		char filepath[30];
		sprintf(filepath, "%s/%06d", fileDir, curFile);
		ifstream file;
		file.open(filepath, ios::in);
		if(file.fail()) errquit("client fstream open file");

		char filename[10];
		sprintf(filename, "%06d", curFile);
		
		bzero(&buf, sizeof(buf));
		while(file.read(buf, sizeof(buf))){
			// Packet(int _seq, bool _fin, int _len, string _filename, bool _fileEnd, char* _data)
			
			Packet tmp(seq, 0, sizeof(buf), filename, 0, buf);
			tmp.calculateCksum();
			tmp.print();
			tmp.send(sockfd);
			// cout << "*\n";
			waitQueue.push_back(tmp);
			// cout << "*\n";
			// delete [] tmp.data;
			seq++;
			bzero(&buf, sizeof(buf));
		}
		Packet temp(seq, 0, file.gcount(), filename, 1, buf);
		temp.calculateCksum();
		temp.print();
		temp.send(sockfd);
		waitQueue.push_back(temp);
		// delete [] tmp.data;
		seq++; curFile++;
		bzero(&buf, sizeof(buf));
	}
}

// void receiving(int sockfd){
// 	int n;	
// 	while(1){
// 		bzero(&rcvBuffer, sizeof(rcvBuffer));
// 		if((n = read(sockfd, rcvBuffer, sizeof(rcvBuffer))) < 0) errquit("write");

// 	}
// }
