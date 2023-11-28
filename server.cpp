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
using namespace std;
#define MAX 10000
#define MAXLINE 30000
#define DATA_SIZE 1000
const int port = 47777;
class Pkt{
public:
    int seq;
    int ACK;
    int fin;
    int cksum;
    string filename;
    int fileEnd;
    string payload;
};
int main(int argc, char *argv[]) {
    stringstream ss;
    ss.str("");
    ss.clear();
	int sockfd; 
    char buf[MAXLINE];
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
    servaddr.sin_port = htons(port); 
       
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    socklen_t clilen;
    
    while(1){
        clilen = sizeof(clientaddr);
        int n;
        if((n = recvfrom(sockfd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr*)&clientaddr, &clilen)) < 0){
            perror("recvfrom");
			break;
        }
        Pkt pkt;
        string response = "";
        ss.str("");
        ss.clear();
        ss << buf;
        string line = "";
        getline(ss, line);
        pkt.seq = atoi(line.c_str());
        getline(ss, line);
        pkt.ACK = atoi(line.c_str());
        getline(ss, line);
        pkt.fin = atoi(line.c_str());
        getline(ss, line);
        pkt.cksum = atoi(line.c_str());
        getline(ss, line);
        pkt.filename = line;
        getline(ss, line);
        pkt.fileEnd = atoi(line.c_str());
        getline(ss, line);
        pkt.payload = line;
        cout << "rcv seq: "<< pkt.seq << ", ACK: " << pkt.ACK << ", fin: " << pkt.fin << ", cksum: " 
            << pkt.cksum << ", filename: " << pkt.filename << ", fileEnd: " << pkt.fileEnd << ", payload: " << pkt.payload << "\n";

        // bzero(&buf, sizeof(buf));
        // sprintf(buf, "receive data!\n");
        response = "receive data! payload: " + pkt.payload + '\n';
        sendto(sockfd, response.c_str(), response.length(), 0, (struct sockaddr*) &clientaddr, sizeof(clientaddr));
    }
	return 0;
}