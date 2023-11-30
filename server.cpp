#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include<fstream>
using namespace std;
#define MAXLINE 300 //60000
#define WINDOW_SIZE 200
#define QUEUE_CAPACITY 10 //1000
#define errquit(m) { perror(m); exit(-1); }

const int port = 47777;
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
    Packet(){
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
    // if(sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) == -1){
    //             perror("sending");
    //             exit(EXIT_FAILURE);
    //         }
	void sendto(int sockfd){
		bzero(&buffer, sizeof(buffer));
		sprintf(buffer, "seq: %d\nACK: %d\nfin: %d\ncksum: %hu\nfilename: %s\nfileEnd: %d\n%s",
			seq, ack, fin, cksum, filename.c_str(), fileEnd, data);
		int n;
		if((n = write(sockfd, buffer, sizeof(buffer))) < 0) errquit("write");
	}
};
uint16_t servCalculateCksum(unsigned char* data, int len){
    unsigned short *ptr = (unsigned short*)data;
    uint16_t cksum = ptr[0];
    int round = len / 2;
    for(int i = 1; i < round; i++){
        cksum = cksum ^ ptr[i];
    }
    return cksum;
}
int main(int argc, char *argv[]) {
    if(argc < 4) {
		return -fprintf(stderr, "usage: %s ... <path-to-store-files> <total-number-of-files> <port>\n", argv[0]);
	}
    string fileDir = argv[1];
	int totalFile = atoi(argv[2]);
    stringstream ss;
    ofstream fileOut;
    ss.str("");
    ss.clear();
	int sockfd; 
    
    // IONBF: not use buffer, each I/O fast write and read  
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0); 
    // const char *hello = "Hello from server"; 
    struct sockaddr_in servaddr, clientaddr; 
    // struct timeval timeout = {2, 0}; //set timeout for 2 seconds
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    // setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));
       
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&clientaddr, 0, sizeof(clientaddr)); 
       
    // Filling server information 
    servaddr.sin_family = AF_INET; // IPv4 
    // servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(strtol(argv[argc-1], NULL, 0));
       
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    socklen_t clilen;
    int seq = 101;
    int lastAckSeq = 0;//Up to client
    while(1){
        clilen = sizeof(clientaddr);
        int n;
        bzero(&buffer, sizeof(buffer));
        if((n = recvfrom(sockfd, buffer, MAXLINE, MSG_WAITALL, (struct sockaddr*)&clientaddr, &clilen)) < 0){
            perror("recvfrom");
			break;
        }
        
        Packet rcvPkt;
        string response = "";
        ss.str("");
        ss.clear();
        ss << buffer;
        string line = "";
        getline(ss, line);
        rcvPkt.seq = atoi(line.c_str());
        getline(ss, line);
        rcvPkt.ack = atoi(line.c_str());
        getline(ss, line);
        rcvPkt.fin = static_cast<bool>(atoi(line.c_str()));
        getline(ss, line);
        rcvPkt.cksum = static_cast<uint16_t>(atoi(line.c_str()));
        getline(ss, line);
        rcvPkt.len = atoi(line.c_str());
        getline(ss, line);
        rcvPkt.filename = line;
        getline(ss, line);
        rcvPkt.fileEnd = static_cast<bool>(atoi(line.c_str()));
        getline(ss, line);
        // strcpy(reinterpret_cast<char*>(rcvPkt.data), line.c_str());
        memcpy(rcvPkt.data, line.c_str(), rcvPkt.len);
        // rcvPkt.data = line;
        // char payload[MAXLINE] = rcvPkt.data;
        string payload(reinterpret_cast<const char*>(rcvPkt.data));
        cout << "rcv seq: "<< rcvPkt.seq << ", ack: " << rcvPkt.ack << ", fin: " << rcvPkt.fin << ", cksum: " 
            << rcvPkt.cksum << "len: "<< rcvPkt.len<<  ", filename: " << rcvPkt.filename << ", fileEnd: " << rcvPkt.fileEnd << ", data: " << rcvPkt.data << "\n";
        // out of order
        if(rcvPkt.seq != (lastAckSeq+1)){
            bzero(&buffer, sizeof(buffer));
            sprintf(buffer, "pkt lost :(\n", rcvPkt.seq);
            Packet temp(seq, lastAckSeq, sizeof(buffer), "", -1, buffer);
            temp.send(sockfd);
            seq++;
        }
        else{/*order right, calculate the cksum*/
            if(servCalculateCksum(rcvPkt.data, rcvPkt.len) == rcvPkt.cksum){
                bzero(&buffer, sizeof(buffer));
                sprintf(buffer, "receive pkt#%d, cksum right!\n", rcvPkt.seq);
                lastAckSeq++;
                Packet temp(seq, lastAckSeq, sizeof(buffer), "", -1, buffer);
                temp.send(sockfd);
                seq++;
                string filePath = fileDir+rcvPkt.filename;
                fileOut.open(filePath);
                if(fileOut.fail()) errquit("fstream open file");
                fileOut << rcvPkt.data;
                fileOut.close();
            }
            else{/*cksum is not right*/
                bzero(&buffer, sizeof(buffer));
                sprintf(buffer, "receive pkt#%d, cksum failed:(\n", rcvPkt.seq);
                Packet temp(seq, lastAckSeq, sizeof(buffer), "", -1, buffer);
                temp.send(sockfd);
                seq++;
            }
        }
        
        // // bzero(&buffer, sizeof(buffer));
        // // sprintf(buffer, "receive data!\n");
        // response = "receive data! data: " + payload + '\n';
        // sendto(sockfd, response.c_str(), response.length(), 0, (struct sockaddr*) &clientaddr, sizeof(clientaddr));
    }
	return 0;
}