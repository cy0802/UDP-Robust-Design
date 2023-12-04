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
#define MAXLINE 1400
#define WINDOW_SIZE 200
#define QUEUE_CAPACITY 20
#define errquit(m) { perror(m); exit(-1); }

const int port = 47777;
char buffer[MAXLINE];
// const char *hello = "Hello from server"; 
struct sockaddr_in servaddr, clientaddr; 
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

	Packet(int _seq, int _ack){
		seq = _seq; ack = _ack;
        fin = 1; cksum = 0; len = -1; filename = "", fileEnd = true; data = NULL;
        
        // seq = _seq; ack = 0; fin = _fin; len = _len;
		// fileEnd = _fileEnd;
		// if(_filename != NULL){
		// 	filename = _filename;
		// } else {
		// 	filename = "";
		// }
		// if(_data != NULL){
		// 	data = new unsigned char[_len];
		// 	memcpy(data, _data, _len);
		// } else {
		// 	data = NULL;
		// }
		isAcked = false;
	};
    Packet(){
		isAcked = false;
        data = NULL;
        filename = "";
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
    ~Packet(){
		// cout << "\t\tdestructor called\tseq: " << seq << "\n";
		if(data != nullptr) delete[] data;
	}
    // if(sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) == -1){
    //             perror("sending");
    //             exit(EXIT_FAILURE);
    //         }
	void send(int sockfd){
		bzero(&buffer, sizeof(buffer));
		sprintf(buffer, "%d\n%d\n%d\n%hu\n%d\n%s\n%d\n%s",
			seq, ack, fin, cksum, len, filename.c_str(), fileEnd, data);
		int n;
		if((n = sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientaddr, sizeof(clientaddr))) < 0) errquit("server write");
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
    system("rm -rf ./serverStore/*");
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
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(strtol(argv[argc-1], NULL, 0));
    // if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){ errquit("connect") } 
	// else cout << "successfully connect to server\n";   
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    socklen_t clilen;
    int seq = 101;
    int lastAckSeq = -1;//Up to client
    bool handshake = false;
    while(1){
        // sprintf(buffer, "hello, world!");
        // Packet temp(seq, lastAckSeq, sizeof(buffer), NULL, -1, buffer);
        //     temp.send(sockfd);
        // cout << "in while loop\n";
        clilen = sizeof(clientaddr);
        int n;
        bzero(&buffer, sizeof(buffer));
        if((n = recvfrom(sockfd, buffer, MAXLINE, MSG_WAITALL, (struct sockaddr*)&clientaddr, &clilen)) < 0){
            perror("recvfrom");
			break;
        }
        // cout << buffer << "\n";
        Packet rcvPkt;
        string response;
        ss.str("");
        ss.clear();
        ss << buffer;
        string line;
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
        // getline(ss, line);
        // cout << "string stream content: "<< ss.str()<<endl;
        // cout << "parse data: "<<line << endl;
        if(rcvPkt.len == 0){
            // cout << "assign data to null!\n";
            rcvPkt.data = NULL;
        }
        else{
            // string payload = "";
            // while(getline(ss, line)){
            //     payload += line;
            // }
            rcvPkt.data = new unsigned char[rcvPkt.len+1];
            
            char payload[MAXLINE];
            bzero(&payload, MAXLINE);
            // bzero(&rcvPkt.data, rcvPkt.len);
            ss.read(payload, rcvPkt.len);
            int bytesRead = ss.gcount();
            // cout << "bytesRead: " << bytesRead << endl;
            for(int i = 0; i < rcvPkt.len; i ++){
                rcvPkt.data[i] = payload[i];
            }
            rcvPkt.data[rcvPkt.len] = '\0';
            // char payload[MAXLINE];
            // bzero(&payload, MAXLINE);
            // bzero(&rcvPkt.data, rcvPkt.len);
            // ss.read(payload, rcvPkt.len);
            // cout << "payload: " << payload << endl;
            // int bytesRead = ss.gcount();
            // cout << "bytesRead: " << bytesRead << endl;
            // for(int i = 0; i < rcvPkt.len; i ++){
            //     rcvPkt.data[i] = payload[i];
            // }
            // memcpy(rcvPkt.data, payload, rcvPkt.len);
        }

        
        // strcpy(reinterpret_cast<char*>(rcvPkt.data), line.c_str());
        // memcpy(rcvPkt.data, line.c_str(), rcvPkt.len);
        // string payload(reinterpret_cast<const char*>(rcvPkt.data));
        // string payload="";
        // if(rcvPkt.data) payload = (char*)rcvPkt.data;
        // cout << "rcv seq: "<< rcvPkt.seq << ", ack: " << rcvPkt.ack << ", fin: " << rcvPkt.fin << ", cksum: " 
        //     << rcvPkt.cksum << "len: "<< rcvPkt.len<<  ", filename: " << rcvPkt.filename << ", fileEnd: " << rcvPkt.fileEnd << ", data: " << rcvPkt.data << "\n";
        // handshake
        if(rcvPkt.data == NULL && !handshake){
            // cout << "in handshake\n";
            lastAckSeq++;
            Packet temp(seq, lastAckSeq);
            temp.send(sockfd);
            seq++;
            handshake = true;
            continue;
        }
        else if(rcvPkt.data == NULL && handshake){/*if has handshaked, but receive NULL data*/
            // cout << "data is null\n";
            bzero(&buffer, sizeof(buffer));
            // sprintf(buffer, "receive pkt#%d, cksum failed:(\n", rcvPkt.seq);
            Packet temp(seq, lastAckSeq);
            temp.send(sockfd);
            seq++;
            continue;
        }
        // cout << "rcv seq: " << rcvPkt.seq << "\n";
        // cout << "rcv seq: "<< rcvPkt.seq << ", ack: " << rcvPkt.ack << ", fin: " << rcvPkt.fin << ", cksum: " 
        //   << rcvPkt.cksum << ", len: "<< rcvPkt.len<<  ", filename: " << rcvPkt.filename << ", fileEnd: " << rcvPkt.fileEnd << ", data: " << rcvPkt.data << "\n";
        // out of order
        if(rcvPkt.seq != (lastAckSeq+1)){
            // cout << "====data out of order====\n";
            // cout << "expect seq: " << (lastAckSeq+1) << endl;
            bzero(&buffer, sizeof(buffer));
            // sprintf(buffer, "pkt lost :(\n", rcvPkt.seq);
            Packet temp(seq, lastAckSeq);
            temp.send(sockfd);
            seq++;
        }
        else{/*order right, calculate the cksum*/
            // testing
            // uint16_t servCksum = servCalculateCksum(rcvPkt.data, rcvPkt.len);
            uint16_t servCksum = rcvPkt.cksum;
            // cout << "server cksum: " << servCksum <<", client cksum: " << rcvPkt.cksum << endl;
            if(servCksum == rcvPkt.cksum){
                bzero(&buffer, sizeof(buffer));
                // sprintf(buffer, "receive pkt#%d, cksum right!\n", rcvPkt.seq);
                // cout << "====have correct cksum, open file====\n";
                lastAckSeq++;
                Packet temp(seq, lastAckSeq);
                temp.send(sockfd);
                seq++;
                string file = rcvPkt.filename.substr(rcvPkt.filename.find_last_of("/")+1); 
                string filePath = fileDir + "/" + file;
                // cout << "filePath: " << filePath << endl;
                fileOut.open(filePath, std::ios_base::app);
                if(fileOut.fail()) errquit("server fstream open file");
                fileOut << rcvPkt.data;
                fileOut.close();
            }
            else{/*cksum is not right*/
                // cout << "====cksum error!====\n";
                bzero(&buffer, sizeof(buffer));
                // sprintf(buffer, "receive pkt#%d, cksum failed:(\n", rcvPkt.seq);
                Packet temp(seq, lastAckSeq);
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