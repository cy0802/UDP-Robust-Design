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
using namespace std;
#define MAXLINE 1400
#define WINDOW_SIZE 200
#define QUEUE_CAPACITY 20
#define errquit(m) { perror(m); exit(-1); }

const int port = 47777;
const int pktNum = 23000;
char buffer[MAXLINE+100];
char recvPktStat[pktNum];
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
	unsigned char* data;

    Packet(char *_data){
        // char _data[1000] = "hello, world!\n";
		if(_data != nullptr){
            // size_t _len = sizeof(_data);
			data = new unsigned char[pktNum + 1];
			// memmove(data, _data, pktNum);
            memcpy(data, _data, pktNum);
			data[pktNum] = '\0';
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
        data = NULL;
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
	void send(int sockfd){
		bzero(&buffer, sizeof(buffer));
		sprintf(buffer, "%s\n", data);
		int n;
		if((n = sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientaddr, sizeof(clientaddr))) < 0){
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
uint16_t servCalculateCksum(unsigned char* data, int len){
    unsigned short *ptr = (unsigned short*)data;
    uint16_t cksum = ptr[0];
    int round = len/2;
    for(int i = 1; i < round; i++){
        cksum = cksum ^ ptr[i];
    }
    return cksum;
}
// uint16_t servCalculateCksum(unsigned char* data, int len){
//     // unsigned short *ptr = (unsigned short*)data;
//     uint16_t cksum = data[0];
//     // int round = len/2;
//     for(int i = 1; i < len; i++){
//         cksum = cksum ^ data[i];
//     }
//     return cksum;
// }
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
    memset(recvPktStat, 0, sizeof(recvPktStat));
    // for(int i = 0; i < pktNum; i++){
        // recvPktStat[i] = '0';
    // }
    // IONBF: not use buffer, each I/O fast write and read  
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0); 
    
    // struct timeval timeout = {2, 0}; //set timeout for 2 seconds
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    struct timeval timeout = {0, 5000}; //set timeout for 5 ms 
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));

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
    timeval start, end;
    gettimeofday(&start, 0);
    int startSec = start.tv_sec;
    // ackFile = 0;
    while(1){
        
        clilen = sizeof(clientaddr);
        int n;
        bzero(&buffer, sizeof(buffer));
        size_t bytesRead = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientaddr, &clilen);
        if (bytesRead == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout occurred
                std::cerr << "======Receive timeout!======\n" << std::endl;
                Packet servStat(recvPktStat);
                // cout << "recvPktStat: " << recvPktStat << endl;
                servStat.send(sockfd);
                servStat.print();
                servStat.deleteData();
                usleep(10000);
            } else {
                // Other error occurred
                // perror("recvfrom failed");
                cout << "can not read file from server\n";
                usleep(10000);
                continue;
            }
        }
        else{/*pkt receive success!*/
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
                // rcvPkt.data = new unsigned char[remainingDataSize + 1];
                rcvPkt.data = new unsigned char[rcvPkt.len + 1];
                ss.read(reinterpret_cast<char*>(rcvPkt.data), rcvPkt.len);

                // cout << "client data len: " << rcvPkt.len << "server data len: " << remainingDataSize << endl;
                rcvPkt.data[rcvPkt.len] = '\0';
            }
            
            if(rcvPkt.data == NULL){/*data has no stuff*/
                continue;
            }else{/*data have stuff*/
                cout << "======seq#" << rcvPkt.seq << "len: " <<rcvPkt.len<<" client data======\n" << rcvPkt.data << endl;
                uint16_t servCksum = servCalculateCksum(rcvPkt.data, rcvPkt.len);
                // uint16_t servCksum = rcvPkt.cksum;
                // uint16_t servCksum = rcvPkt.calculateCksum();
                cout << "server cksum: " << servCksum <<", client cksum: " << rcvPkt.cksum << endl;
                if(servCksum == rcvPkt.cksum){
                    // ackFile++;
                    bzero(&buffer, sizeof(buffer));
                    // sprintf(buffer, "receive pkt#%d, cksum right!\n", rcvPkt.seq);
                    cout << "====have correct cksum, open file====\n";
                    if(recvPktStat[rcvPkt.seq] == '1'){/*have receive*/
                        continue;
                    }
                    // lastAckSeq++;
                    recvPktStat[rcvPkt.seq] = '1';/*means has receive #seq pkt*/

                    // string file = rcvPkt.filename.substr(rcvPkt.filename.find_last_of("/")+1); 
                    // string filePath = fileDir + "/" + file;
                    string filePath = fileDir + "/" + rcvPkt.filename;
                    rcvPkt.offset;
                    cout << "filePath: " << filePath << endl;
                    fileOut.open(filePath, std::ios::binary | std::ios_base::app);
                    if(fileOut.fail()) errquit("server fstream open file");
                     // Write rcvPkt.data to the file
                    fileOut.write(reinterpret_cast<const char*>(rcvPkt.data), rcvPkt.len);

                    // Close the file
                    fileOut.close();
                }
                else{/*cksum is not right*/
                    cout << "====cksum error!====\n";
                    // bzero(&buffer, sizeof(buffer));
                    // // sprintf(buffer, "receive pkt#%d, cksum failed:(\n", rcvPkt.seq);
                    // Packet temp(seq, lastAckSeq);
                    // temp.send(sockfd);
                    // seq++;
                }
            }

        }
        gettimeofday(&end, 0);
        int elapsed_time = end.tv_sec - startSec;
        // if(elapsed_time > 5){
        //     cout << "===========send server stat===============\n";
        //     // startSec = end.tv_sec;
        //     // Packet servStat(recvPktStat);
        //     // // cout << "recvPktStat: " << recvPktStat << endl;
        //     // servStat.send(sockfd);
        //     // servStat.print();
        // }
    }
    return 0;
}