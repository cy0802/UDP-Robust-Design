#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <bitset>
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

class Packet{
public:
	int seq;
	uint16_t cksum;
    int offset;
	int len; // only length of data
	string filename;
	bool fileEnd;
	unsigned char* data;

	// copy constructor
	Packet(const Packet &pkt){
		// cout << "\t\tcopy constructor called\nseq: " << pkt.seq << "\n";
		this->seq = pkt.seq; this->cksum = pkt.cksum; this->len = pkt.len;
		this->filename = pkt.filename; this->fileEnd = pkt.fileEnd; this->offset = pkt.offset;
		if(pkt.data != nullptr){
			this->data = new unsigned char[pkt.len + 1];
			memmove(this->data, pkt.data, pkt.len);
			this->data[len] = '\0';
		} else {
			this->data = nullptr;
		}
	}

	Packet(int _seq, int _offset, int _len, char* _filename, bool _fileEnd, char* _data){
		// cout << "\t\tconstructor with multiple parameters called\tseq: " << seq << "\n";
		seq = _seq; len = _len;
		fileEnd = _fileEnd; offset = _offset;
		if(_filename != nullptr){
			filename = _filename;
		} else {
			filename = "";
		}
		if(_data != nullptr){
			data = new unsigned char[_len + 1];
			memmove(data, _data, _len);
			data[_len] = '\0';
			len ++;
		} else {
			data = nullptr;
		}
	};

	// move constructor
	// Packet(Packet&& pkt){ // : data(nullptr)
	// 	// cout << "\t\tmove constructor called\tseq: " << pkt.seq << "\n";
	// 	data = pkt.data;
	// 	pkt.data = nullptr;
	// }

	// move assignment operator
	Packet& operator=(Packet&& other){
		// cout << "\t\tmove assignment operator called\tseq: " << other.seq << "\n";
		if(this != &other){
			// delete [] data;
			data = other.data;
			other.data = nullptr;
			len = other.len; seq = other.seq; offset = other.offset;
			fileEnd = other.fileEnd; filename = other.filename; cksum = other.cksum;
		}
		return *this;
	}

    // overload assignment
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
		// unsigned short *ptr = (unsigned short*)data;
		cksum = data[0];
		// int round = len / 2;
		for(int i = 1; i < len; i++){
			cksum = cksum ^ data[i];
		}
	}
	void printDetail(){
		printf("====================================================\n");
		printf("seq: %d\ncksum: %hu\noffset: %d\nlen: %d\nfilename: %s\nfileEnd: %d\n%s\n",
			seq, cksum, offset, len, filename.c_str(), fileEnd, data);
		printf("====================================================\n");
		// cout << " seq: " << seq << "\n";
	}
    void print(){
        cout << "seq: " << seq << "\n";
		// cout << "seq: " << seq << "len: "<< len<<"\n";
		cout << "====data====\n" << data << "\n";
    }
	void send(int sockfd){
		bzero(&buffer, sizeof(buffer));
		// sprintf(sendBuffer, "seq: %d\nACK: %d\nfin: %d\ncksum: %hu\nfilename: %s\nfileEnd: %d\n%s",
		// 	seq, ack, fin, cksum, filename.c_str(), fileEnd, data);
		sprintf(buffer, "%d\n%hu\n%d\n%d\n%s\n%d\n%s",
			seq, cksum, offset, len, filename.c_str(), fileEnd, data);
		int n;
		if((n = write(sockfd, buffer, sizeof(buffer))) < 0) errquit("client write");
		// cout << "sent\n";
	}
    void isACKed(){
        delete [] data;
        data = nullptr;
    }
};

void readFile();
vector<Packet> sendQueue;
static int sockfd = -1;
static struct sockaddr_in sin;
int totalFile;
char* fileDir;


int main(int argc, char *argv[]){
    if(argc < 3) {
		return -fprintf(stderr, "usage: %s ... <port> <ip>\n", argv[0]);
	}

	fileDir = argv[1];
	totalFile = atoi(argv[2]);

    memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-2], nullptr, 0));
	if(inet_pton(AF_INET, argv[argc-1], &sin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[1]);
	}
	socklen_t len = sizeof(sin);

	if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){ errquit("client socket");}
	else cout << "successfully create socket\n";

	struct timeval timeout = {0, 5000}; //set timeout for 5 ms 
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));

	if(connect(sockfd, (struct sockaddr *) &sin, sizeof(sin)) < 0){ errquit("client connect") } 
	else cout << "successfully connect to server\n";

    readFile();

    while(1){
        
        // send
        bool allsent = true;
        for(auto it = sendQueue.begin(); it < sendQueue.end(); it++){
            if(it->data == nullptr) continue;
            allsent = false;
            it->send(sockfd);
            it->print();
            usleep(75); // sleep for 0.75ms
        }
        if(allsent) break;

        // rcv bitset: haven't test
        int n;
        char rcvbuffer[23000];
		char bset[23000];
		bzero(&rcvbuffer, sizeof(rcvbuffer));
		while(1){
			if((n = read(sockfd, rcvbuffer, sizeof(rcvbuffer))) < 0){
				cout << "timeout" << "\n";
				break;
			} else {
				bzero(&bset, sizeof(bset));
				memcpy(bset, rcvbuffer, sizeof(rcvbuffer));
			}
		}
		cout << "rcv from server: " << bset << endl;
        for(int i = 0; i < sendQueue.size(); i++){
			if(bset[i] == '0' && sendQueue[i].data == nullptr){
				// if the packet is marked not received but the data is deleted, read it again
				char* filepath;
				sprintf(filepath, "%s/%s", fileDir, sendQueue[i].filename);
				fstream file;
				file.open(filepath, ios::in);
				if(file.fail()) errquit("client fstream open file");
				
				file.seekg(sendQueue[i].offset);
				char buf[MTU]; bzero(&buf, sizeof(buf));
				file.read(buf, sendQueue[i].len);
				sendQueue[i].data = new unsigned char[sendQueue[i].len];
				memcpy(sendQueue[i].data, buf, sendQueue[i].len);
			}
            if(bset[i] == '1' && sendQueue[i].data != nullptr) sendQueue[i].isACKed();
        }
    }
    close(sockfd);
    return 0;
}

void readFile(){
    int seq = 0;
    char buf[MTU];
    for(int curFile = 0; curFile < totalFile; curFile++){
        char filepath[30];
        char filename[10];
        sprintf(filename, "%06d", curFile);
        sprintf(filepath, "%s/%06d", fileDir, curFile);
        fstream file;
        file.open(filepath, ios::in);
        if(file.fail()) errquit("client fstream open file");

        bzero(&buf, sizeof(buf));
        int offset = file.tellg();
        while(file.read(buf, sizeof(buf))){
            sendQueue.push_back(Packet(seq, offset, sizeof(buf), filename, 0, buf));
            (sendQueue.end() - 1)->calculateCksum();
            offset = file.tellg();
            seq++;
        }
        sendQueue.push_back(Packet(seq, offset, file.gcount(), filename, 1, buf));
        (sendQueue.end() - 1)->calculateCksum();
        seq++;
    }
}