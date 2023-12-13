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
#include<cerrno>
#include<filesystem>
using namespace std;
namespace fs = std::filesystem;
#define MAXLINE 1400
#define WINDOW_SIZE 200
#define QUEUE_CAPACITY 20
#define errquit(m) { perror(m); exit(-1); }

const int port = 47777;
const int pktNum = 23000;
char buffer[MAXLINE+100];
char rcvBuffer[MAXLINE+100];
// short recvPktStat[pktNum];
char recvPktStat[pktNum];
char* clientFiles[1010];
// const int ackFile = 0;
// const char *hello = "Hello from server"; 
struct sockaddr_in servaddr, clientaddr; 
class Packet{
public:
	int seq;
	uint16_t cksum;
    int offset;
	int len; // only length of data
	string filename;
	bool fileEnd;
	char* data;

    Packet(char* _data){
        // char _data[1000] = "hello, world!\n";
		if(_data != nullptr){
            // size_t _len = sizeof(_data);
			data = new char[pktNum];
			// memmove(data, _data, pktNum);
            // memcpy(data, _data, pktNum);
            for(int i = 0; i < pktNum-1; i++){
                // if(_data[i] == 1){
                //     data[i] = '1';
                // }
                // else{
                //     data[i] = '0';
                // }
                if(_data[i] == '1'){
                    data[i] = '1';
                }else if(_data[i] == '0'){
                    data[i] = '0';
                }
                // data[i] = _data[i]; 
            }
			data[pktNum-1] = '\0';
		} else {
			data = nullptr;
		}
	};
	// Packet(int _seq, int _ack){
    //     // char _data[1000] = "hello, world!\n";
	// 	seq = _seq; ack = _ack;
    //     fin = 1; cksum = 0; len = -1; filename = "", fileEnd = true; data = NULL;
        
	// 	isAcked = false;
	// };
    Packet(){
        data = nullptr;
	};
	// void calculateCksum(){
	// 	unsigned short *ptr = (unsigned short*)data;
	// 	cksum = ptr[0];
	// 	int round = len / 2;
	// 	for(int i = 1; i < round; i++){
	// 		cksum = cksum ^ ptr[i];
	// 	}
	// }
	void print(){
		printf("====================================================\n");
		printf("%s\n", data);
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
	void send(int sockfd, int offset, int len){
        int bound = max(offset, MAXLINE);
        char sendData[MAXLINE+10];
        // cout << "=======server Send========\n";
        
        bzero(&sendData, sizeof(sendData));
    	bzero(&rcvBuffer, sizeof(rcvBuffer));
        for(int i = 0; i < len; i ++){
            sendData[i] = data[i+offset];
        }
        sendData[len] = '\0';
        // cout << "===len: " << len << ", offset: " << offset << "====data====\n" << sendData << '\n';
		// cout << "===len: " << len << ", offset: " << offset << endl;
        sprintf(rcvBuffer, "%d\n%d\n%s", len, offset, sendData);
    
		int n;
        // cout << "======offset: " << offset << " data========\n" << data << endl;
		if((n = sendto(sockfd, rcvBuffer, sizeof(rcvBuffer), 0, (struct sockaddr *) &clientaddr, sizeof(clientaddr))) < 0){
            cout << "======can not send to client, sleep a while...======\n";
            usleep(10000);
            // errquit("server write");
        }

    }
    void deleteData(){
        delete [] data;
        data = nullptr;
    }
};
uint16_t servCalculateCksum(char* data, int len){
    unsigned short *ptr = (unsigned short*)data;
    uint16_t cksum = ptr[0];
    int round = len/2;
    for(int i = 1; i < round; i++){
        cksum = cksum ^ ptr[i];
    }
    return cksum;
}
void printBitset(){
    cout << "===================servStat===============\n";
    for(int i = 0; i < pktNum-1; i++){
        if(recvPktStat[i] == '1'){
            cout << "1";
        }else{
            cout << "0";
        }
    }
    cout << '\n';
}
int main(int argc, char *argv[]) {
    if(argc < 4) {
		return -fprintf(stderr, "usage: %s ... <path-to-store-files> <total-number-of-files> <port>\n", argv[0]);
	}
<<<<<<< HEAD
    system("rm -rf ./serverStore/*");
    char* fileDir = argv[1];
=======
    // system("rm -rf ./serverStore/*");
    string fileDir = argv[1];
>>>>>>> 3808867b77cbea142f3d01f066af92e6972892da
	int totalFile = atoi(argv[2]);
    stringstream ss;
    fstream fileOut;
    ss.str("");
    ss.clear();
	int sockfd; 
    memset(clientFiles, 0, sizeof(clientFiles));
    memset(recvPktStat, '0', sizeof(recvPktStat));
    // IONBF: not use buffer, each I/O fast write and read  
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0); 
    
    // struct timeval timeout = {2, 0}; //set timeout for 2 seconds
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    // struct timeval timeout = {0, 5000}; //set timeout for 5 ms 
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
    // bool handshake = false;
    timeval start, end, fin;
    gettimeofday(&start, 0);
    int startSec = start.tv_sec;
    int startUsec = start.tv_usec;
    int ackFile = 0;
	int largestAckSeq = 0;
    int ack_counter = 0;
    while(1){
    
        clilen = sizeof(clientaddr);
        int n;
        bzero(&buffer, sizeof(buffer));
        size_t bytesRead = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientaddr, &clilen);
        if (bytesRead == -1) {
			perror("recvfrom");
			break;
        }
        // cout << "========Serv Recv pkt========\n";
        gettimeofday(&end, 0);
        // int elapsed_time = end.tv_sec - startSec;
        float elapsed_time = (end.tv_sec - startSec) + ((end.tv_usec-startUsec) / 1000000.0);
        //cout << "start: " << startSec << "\tend: " << end.tv_sec << "\n";
        if(elapsed_time > 0.2){/*0.2s*/
            cout << "===========send server stat===============\n";
            startSec = end.tv_sec;
            startUsec = end.tv_usec;
            Packet servStat(recvPktStat);
            int offset = 0, len = 0;
            while(offset < pktNum){
                len = min(MAXLINE, pktNum-offset);/*last pkt len: < MAXLINE, others len: == MAXLINE*/
                servStat.send(sockfd, offset, len);
                offset += MAXLINE;
            }
            cout << "ACK file: " << ackFile << ", Total files: " << totalFile << endl;
            // cout << "recvPktStat: " << recvPktStat << endl;
            // int cnt = 5;
            // while(cnt--) servStat.send(sockfd);
            // servStat.print();
            // printBitset();
			// usleep(2000); // sleep for 1ms
        }
        // bool endServ = false;
		if(ackFile == totalFile){/*TODO: check that we can receive all file(1) before last fileEnd*/
			bool recvAll = true;
            // cout << "====largest ack: "<< largestAckSeq << "=====\n";
			for(int i = 0; i <= largestAckSeq; i++){
				if(recvPktStat[i] != '1'){
					recvAll = false;
					break;
				}
			}
			if(recvAll){
				break;
			}
        }
		Packet rcvPkt;
        string response;
        ss.str("");
        ss.clear();
        ss << buffer;
        ss >> rcvPkt.seq >> rcvPkt.cksum >> rcvPkt.offset >> rcvPkt.len >> rcvPkt.filename >> rcvPkt.fileEnd;
        if(!ss.eof()){
            char ch;
            ss.get(ch);/*read "\n" at the begining of ss(in front of <rcvPkt.data>)*/
            // size_t remainingDataSize = strlen(buffer) - ss.tellg();
            // rcvPkt.data = new char[remainingDataSize + 1];
            rcvPkt.data = new char[rcvPkt.len + 1];
            ss.read(rcvPkt.data, rcvPkt.len);

            // cout << "client data len: " << rcvPkt.len << "server data len: " << remainingDataSize << endl;
            rcvPkt.data[rcvPkt.len] = '\0';
        }
        
        if(rcvPkt.data == NULL){/*data has no stuff*/
            continue;
        }else{
<<<<<<< HEAD
            // uint16_t servCksum = servCalculateCksum(rcvPkt.data, rcvPkt.len);
            // if(servCksum == rcvPkt.cksum){
                // bzero(&buffer, sizeof(buffer));
=======
            uint16_t servCksum = servCalculateCksum(rcvPkt.data, rcvPkt.len);
            servCksum = rcvPkt.cksum;
            if(servCksum == rcvPkt.cksum){
                bzero(&buffer, sizeof(buffer));
>>>>>>> 3808867b77cbea142f3d01f066af92e6972892da
                // sprintf(buffer, "receive pkt#%d, cksum right!\n", rcvPkt.seq);
                
                if(recvPktStat[rcvPkt.seq] == '1'){/*have receive*/
                    continue;
                }
                if(rcvPkt.fileEnd){/*if true, counter++*/
                    ackFile++;
                    if(rcvPkt.seq > largestAckSeq){
                        largestAckSeq = rcvPkt.seq;
                    }
                }
                // cout << "=====seq#" << rcvPkt.seq << " have correct cksum, open file====\n";
                cout << "======seq#" << rcvPkt.seq<< " offset: " << rcvPkt.offset<< " len: " <<rcvPkt.len<<" client data======\n" << rcvPkt.data << endl;
                
                recvPktStat[rcvPkt.seq] = '1';/*means has receive #seq pkt*/
                ack_counter++;
                // string file = rcvPkt.filename.substr(rcvPkt.filename.find_last_of("/")+1); 
                // string filePath = fileDir + "/" + file;

                // string filePath = fileDir + "/" + rcvPkt.filename;
                char filePath[100];
                sprintf(filePath, "%s/%s", fileDir, rcvPkt.filename.c_str());
                cout << "filePath: " << filePath << endl;
                // unsigned char* ptr = new unsigned char[] 
                // int fp = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                // if (fp == -1) fail("open");
                // write(fp, reinterpret_cast<const char*>(rcvPkt.data), filesize);
                // ptr += filesize;
                // close(fp);

                fileOut.open(filePath, std::ios::in | std::ios::out | std::ios::ate);
                if(!fileOut){/*if file not exist, create it*/
                    // errquit("server fstream open file");
                    fileOut.open(filePath, std::ios::in | std::ios::out | std::ios::trunc);
                    fileOut.seekp(rcvPkt.offset, std::ios::beg);
                    fileOut.write(rcvPkt.data, rcvPkt.len);
                    fileOut.close();
                }else{
                    fileOut.seekp(rcvPkt.offset, std::ios::beg);
                    fileOut.write(rcvPkt.data, rcvPkt.len);
                    fileOut.close();
                }
            // }
            // else{
            //     cout <<"cksum error!\n";
            // }
        }
        gettimeofday(&fin, 0);
        int total_sec = fin.tv_sec - start.tv_sec;
        if(total_sec > 590){
            break;
        } 
    }
    cout << "======Server End======\n";
    printBitset();
    cout << "======totoal ack " << ack_counter << " pkts=====\n";
    return 0;
}