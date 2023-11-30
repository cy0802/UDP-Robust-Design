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
#define MAXLINE 300 //60000
#define WINDOW_SIZE 200
#define QUEUE_CAPACITY 10 //1000
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
		string _raw = raw;
		cout << raw << endl;
		stringstream ss; ss.clear();
		string tmp;
		ss << raw;
		ss >> tmp >> seq >> tmp >> ack >> tmp >> fin >> tmp >> cksum;
		len = 0;
		filename = "";
		data = NULL;
	}

	Packet(int _seq, bool _fin, int _len, char* _filename, bool _fileEnd, char* _data){
		seq = _seq; ack = 0; fin = _fin; len = _len;
		fileEnd = _fileEnd;
		if(_filename != NULL){
			filename = _filename;
		} else {
			filename = "";
		}
		if(_data != NULL){
			data = new unsigned char[_len];
			memcpy(data, _data, _len);
		} else {
			data = NULL;
		}
		isAcked = false;
	};

	~Packet(){
		delete[] data;
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
		printf("====================================================\n");
		printf("seq: %d\nACK: %d\nfin: %d\ncksum: %hu\nfilename: %s\nContent Length: %d\nfileEnd: %d\n%s\n",
			seq, ack, fin, cksum, filename.c_str(), len, fileEnd, data);
		printf("====================================================\n");
	}
	void send(int sockfd){
		bzero(&sendBuffer, sizeof(sendBuffer));
		// sprintf(sendBuffer, "seq: %d\nACK: %d\nfin: %d\ncksum: %hu\nfilename: %s\nfileEnd: %d\n%s",
		// 	seq, ack, fin, cksum, filename.c_str(), fileEnd, data);
		sprintf(sendBuffer, "%d\n%d\n%d\n%hu\n%s\n%d\n%s",
			seq, ack, fin, cksum, filename.c_str(), fileEnd, data);
		int n;
		if((n = write(sockfd, sendBuffer, sizeof(sendBuffer))) < 0) errquit("write");
		cout << "sent\n";
	}
};

static int sockfd = -1;
static struct sockaddr_in sin;
vector<Packet> waitQueue;
int cnt = WINDOW_SIZE;
int totalFile;
char* fileDir;

void send10File(int sockfd);
Packet rcv(int sockfd);

int main(int argc, char *argv[]) {
	
	if(argc < 3) {
		return -fprintf(stderr, "usage: %s ... <port> <ip>\n", argv[0]);
	}

	fileDir = argv[1];
	totalFile = atoi(argv[2]);

	srand(time(0) ^ getpid());

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-2], NULL, 0));
	if(inet_pton(AF_INET, argv[argc-1], &sin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[1]);
	}
	socklen_t len = sizeof(sin);

	if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){ errquit("socket");}
	else cout << "successfully create socket\n";

	if(connect(sockfd, (struct sockaddr *) &sin, sizeof(sin)) < 0){ errquit("connect") } 
	else cout << "successfully connect to server\n";

	// hand shaking
	// Packet(int _seq, bool _fin, int _len, char* _filename, bool _fileEnd, char* _data)
	Packet handshaking = Packet(0, 1, 0, NULL, 0, NULL);
	handshaking.print();
	while(1){
		handshaking.send(sockfd);
		bzero(&handshaking, sizeof(handshaking));
		Packet tmp = rcv(sockfd);
		if(tmp.ack == 0){ 
			servSeq = tmp.seq;
			break;
		}
	}
	int n;
	// start transfer data
	while(1){
		send10File(sockfd);

		// rcv ACK
		while(1){
			// TODO: set socket option: timeout
			bzero(&rcvBuffer, sizeof(rcvBuffer));
			if((n = read(sockfd, rcvBuffer, sizeof(rcvBuffer))) < 0){
				cout << "timeout\n";
				break;
			}
			Packet tmp(rcvBuffer);
			if(tmp.ack > lastACK && tmp.ack < seq) lastACK = tmp.ack;
		}
		
		while(1){
			if(waitQueue.empty()) break;
			auto it = waitQueue.begin();
			if(it->seq < lastACK) waitQueue.erase(it);
			else break;
		}

	}

	close(sockfd);
}

Packet rcv(int sockfd) {
	int n;
	bzero(&rcvBuffer, sizeof(rcvBuffer));
	if((n = read(sockfd, rcvBuffer, sizeof(rcvBuffer))) < 0) errquit("read");
	return Packet(rcvBuffer);
}

void send10File(int sockfd){
	// send pkt in waitQueue
	for(auto it = waitQueue.begin(); it < waitQueue.end(); it++){
		it->send(sockfd);
	}
	// send 10 files a time
	char buf[MAXLINE - 50];
	int cnt = 10, n;
	while(cnt-- && curFile < totalFile){
		char* filename;
		sprintf(filename, "%s%06d", fileDir, curFile);
		fstream file;
		file.open(filename);
		if(file.fail()) errquit("fstream open file");
		
		bzero(&buf, sizeof(buf));
		while(file.read(buf, sizeof(buf))){
			// Packet(int _seq, bool _fin, int _len, string _filename, bool _fileEnd, char* _data)
			Packet tmp(seq, 0, sizeof(buf), filename, 0, buf);
			tmp.calculateCksum();
			tmp.print();
			tmp.send(sockfd);
			waitQueue.push_back(tmp);
			seq++;
			bzero(&buf, sizeof(buf));
		}
		Packet tmp(seq, 0, file.gcount(), filename, 1, buf);
		tmp.calculateCksum();
		tmp.print();
		tmp.send(sockfd);
		waitQueue.push_back(tmp);
		seq++; curFile++;
		bzero(&buf, sizeof(buf));
	}
}

void receiving(int sockfd){
	int n;	
	while(1){
		bzero(&rcvBuffer, sizeof(rcvBuffer));
		if((n = read(sockfd, rcvBuffer, sizeof(rcvBuffer))) < 0) errquit("write");

	}
}
