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
#include <arpa/inet.h>

using namespace std;
#define MAXLINE 30000
#define PKTNUM 10000
class Pkt{
public:
    Pkt(int seq, int ACK, int fin, int cksum, string filename, int fileEnd, string payload)
        : seq(seq), ACK(ACK), fin(fin), cksum(cksum), filename(filename), fileEnd(fileEnd), payload(payload){};
    int seq;
    int ACK;
    int fin;
    int cksum;
    string filename;
    int fileEnd;
    string payload;
};

const char* SERVER_IP = "127.0.0.1";
const int PORT = 47777;
// client send extreme large buffer to server
// after server get client packet, server send pkt contain time to client   
int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    socklen_t serverlen = sizeof(servaddr);
    char buf[MAXLINE];
    // IONBF: not use buffer, each I/O fast write and read  
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	int n = 0;
    long long int send_byte = 0;
	struct timeval send_time;
    // float latency = 0;
    // float bandwidth = 0;
    // float latency;
    bzero(&servaddr, sizeof(servaddr));
    // Set up server address
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT); // Use any available port
    // int inet_pton(int af, const char *src, void *dst);
	if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) <= 0) {
        perror("Error converting IP address");
        exit(EXIT_FAILURE);
    }
    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    Pkt sendPkt = Pkt(1, 2, 0, 123, "example.txt", 1, "This is the payload.");
    stringstream ss;
    ss.str("");
    ss.clear();
    

    string concatStr = to_string(sendPkt.seq) +"\n"+to_string(sendPkt.ACK) +"\n"+ to_string(sendPkt.fin) +"\n"+ to_string(sendPkt.cksum) +"\n"+ sendPkt.filename +"\n"+ to_string(sendPkt.fileEnd) +"\n"+ sendPkt.payload+"\n";

    send_byte = sendto(sockfd, concatStr.c_str(), concatStr.length(), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    // printf("send %lld byte to server\n", send_byte);
    if(send_byte < 0){
            perror("send failed");
            close(sockfd);
            return 0;
    }
    
    bzero(&buf, sizeof(buf));
    // Receive data from the server (replace this with your actual data)
    // printf("====receive====\n");
    // MSG_WAITALL: wait for all MAX_BUF_SIZE bytes arrive
    n = recvfrom(sockfd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &serverlen);
    if(n < 0){
            perror("receive failed");
            close(sockfd);
            return 0;
    }
    buf[n] = '\0';
    printf("%s", buf);
    close(sockfd);

    return 0;
}
