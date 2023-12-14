#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <algorithm>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define MTU 1400
#define errquit(m) { perror(m); exit(-1); }
using namespace std;

char buffer[MTU + 100];
char rcvflag[25000];
char* fileDir;
struct sockaddr_in servaddr, clientaddr; 
socklen_t clilen;
bool over = false;
int sockfd; 
bool clientAccepted = false;
mutex lock_;

class Packet{
public:
	int seq;
	uint16_t cksum;
    int offset;
	int len; // only length of data
	char filename[10];
	int fileEnd;
	char data[MTU];
	Packet(){}
	Packet(char* raw){
		cout << raw << endl;
		sscanf(raw, "%d\n%hu\n%d\n%d\n%s\n%d\n", &seq, &cksum, &len, filename, &fileEnd);
		int cnt = 0;
		int i;
		for(i = 0; i < strlen(raw); i++){
			if(raw[i] == '\n'){
				cnt++;
			}
			if(cnt == 6){
				i++;
				break;
			}
		}
		memcpy(data, raw + i, len);
		for(int i = 0; i < len; i++){
			cout << data[i];
		}
		cout << endl;
		// stringstream ss; ss.clear(); ss << raw;
		// char tmp; // \n
		// ss >> seq >> cksum >> offset >> len >> filename >> fileEnd;
		// bzero(data, sizeof(data));
		// ss.ignore();
		// for(int i = 0; i < len; i++){
		// 	ss >> tmp;
		// 	data[i] = tmp;
		// }
		// // ss.read(data, len);
		// data[len] = '\0';
	}

	void printDetail(){
		printf("====================================================\n");
		printf("seq: %d\ncksum: %hu\noffset: %d\nlen: %d\nfilename: %s\nfileEnd: %d\n%s\n",
			seq, cksum, offset, len, filename, fileEnd, data);
		printf("====================================================\n");
		// cout << " seq: " << seq << "\n";
	}
};

void sendACK(){
	while(!clientAccepted) usleep(100);
	char buf[MTU];
	char tmp[MTU];
	while(1){
		lock_.lock();
		for(int offset = 0; offset < 23000; offset += MTU-20){
			bzero(buf, sizeof(buf));
			bzero(tmp, sizeof(tmp));
			// cout << rcvflag + offset << endl;
			strncpy(tmp, rcvflag + offset, MTU-20);
			sprintf(buf, "%d\n%d\n%s", MTU-20, offset, tmp);
			// cout << "=============== sendACK =====================\n";
			// cout << buffer << "\n=============================================\n";
			if(sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &clientaddr, clilen) < 0)
				errquit("server thread 2 sendto");
		}
		lock_.unlock();
		usleep(500000); // sleep for 0.5s
		if(over) break;
	}
}

ofstream files[1005];

int main(int argc, char* argv[]){
	system("rm -r ./serverStore/*");
	if(argc < 4) {
		return -fprintf(stderr, "usage: %s ... <path-to-store-files> <total-number-of-files> <port>\n", argv[0]);
	}
    // system("rm -rf ./serverStore/*");
    fileDir = argv[1];
	int totalFile = atoi(argv[2]);
	
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) errquit("server create socket");
	bzero(&servaddr, sizeof(servaddr));
	bzero(&clientaddr, sizeof(clientaddr));
	memset(rcvflag, '0', sizeof(rcvflag));
	servaddr.sin_family = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(strtol(argv[argc-1], NULL, 0));
	if(bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) errquit("server bind");
	clilen = sizeof(clientaddr);

	for(int i = 0; i < totalFile; i++){
		char filename[10];
		sprintf(filename, "%06d", i);
		string filepath = fileDir;
		filepath = filepath + "/" + filename;
		files[i].open(filepath, ios::binary);
		if(files[i].fail()) errquit("server open file");
	}
	
	thread sendack(sendACK);
	sendack.detach();
	
	int endSeq = -1;
	// int cnt = 0;
	while(1){
		bzero(&buffer, sizeof(buffer));
		if(recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientaddr, &clilen) < 0) 
			errquit("server recvfrom");
		clientAccepted = true;
		Packet rcvedPkt(buffer);
		
		// rcvedPkt.printDetail();
		// cout << "========== rcv seq " << rcvedPkt.seq << " ===================\n";
		if(rcvflag[rcvedPkt.seq] == '1') continue;

		// here
		if(rcvedPkt.seq % 200 == 17){ 
			cout << "server ================================================\n";
			// rcvedPkt.printDetail();
			// cout << rcvedPkt.data << endl;
			for(int i = 0; i < rcvedPkt.len; i++){
				cout << rcvedPkt.data[i];
			}
			cout << endl;
			cout << "=======================================================\n";
		}
		

		// cnt++; 
		// if(cnt % 2 == 0){
		// 	cout << "Press anything to continue\n";
		// 	char tmp; cin >> tmp;
		// }

		lock_.lock();
		rcvflag[rcvedPkt.seq] = '1';
		lock_.unlock();

		int filenum = atoi(rcvedPkt.filename);
		files[filenum].seekp(rcvedPkt.offset, ios::beg);
		files[filenum].write(rcvedPkt.data, rcvedPkt.len);
		
		if(atoi(rcvedPkt.filename) == totalFile - 1 && rcvedPkt.fileEnd) endSeq = rcvedPkt.seq;

		if(endSeq != -1){
			bool allrcv = true;
			for(int i = 0; i < endSeq; i++){
				if(rcvflag[i] == '0'){
					allrcv = false;
					break;
				}
			}
			if(allrcv){
				over = true;
				break;
			}
		}
	}
	for(int i = 0; i < totalFile; i++) files[i].close();
	usleep(10000);
	ifstream file;
	string filepath = fileDir;
	filepath = filepath + "/000000";
	file.open(filepath);
	if(file.fail()) errquit("server read file");
	cout << "server 000000 ============================================\n";
	cout << file.rdbuf();
	cout << "\n==========================================================\n";

	return 0;
}