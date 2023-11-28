/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <queue>
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

char buffer[MAXLINE];

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

	Packet(int _seq, bool _fin, int _len, char* _filename, bool _fileEnd, char* _data){
		seq = _seq; ack = 0; fin = _fin; len = _len;
		filename = _filename; fileEnd = _fileEnd;
		filename = _filename;
		data = new unsigned char[_len];
		memcpy(data, _data, _len);
		isAcked = false;
	};
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
		bzero(&buffer, sizeof(buffer));
		sprintf(buffer, "seq: %d\nACK: %d\nfin: %d\ncksum: %hu\nfilename: %s\nfileEnd: %d\n%s",
			seq, ack, fin, cksum, filename.c_str(), fileEnd, data);
		int n;
		if((n = write(sockfd, buffer, sizeof(buffer))) < 0) errquit("write");
	}
};

static int s = -1;
static struct sockaddr_in sin;
queue<Packet> sendQueue;
queue<Packet> waitQueue;
int cnt = WINDOW_SIZE;
int totalFile;
char* fileDir;

void sending();
void receiving();
void prepare();

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

	// if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	// 	err_quit("socket");

	prepare();
	

	close(s);
}

void prepare(){
	int seq = 1;
	int curFile = 0;
	char buf[MAXLINE];
	for(curFile = 0; curFile < totalFile; curFile++){
		char* filename;
		sprintf(filename, "%s%06d", fileDir, curFile);
		fstream file;
		file.open(filename);
		if(file.fail()) errquit("fstream open file");
		
		bzero(&buf, sizeof(buf));
		while(file.read(buf, sizeof(buf))){
			while(sendQueue.size() >= QUEUE_CAPACITY) sleep(1);
			// Packet(int _seq, bool _fin, int _len, string _filename, bool _fileEnd, char* _data)
			Packet tmp(seq, 0, sizeof(buf), filename, 0, buf);
			tmp.calculateCksum();
			tmp.print();
			// TODO: lock
			sendQueue.push(tmp);
			seq++;
			bzero(&buf, sizeof(buf));
		}
		Packet tmp(seq, 0, file.gcount(), filename, 1, buf);
		tmp.calculateCksum();
		tmp.print();
		// TODO: lock
		sendQueue.push(tmp);
		seq++;
		bzero(&buf, sizeof(buf));
	}
}