#include<iostream>
#include<cstdio>
#include<fstream>
#include<string>
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/time.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#define MAXLINE 60000
using namespace std;
int main(int argc, char *argv[]){
    char* path = argv[1];
    int numOfFile = stoi(argv[2]);
    int port = stoi(argv[3]);
    char* address = argv[4];
    
    cout << "path: " << path << endl;
    cout << "number of file: " << numOfFile << endl;
    cout << "port: " << port << endl;
    cout << "server address: " << address << endl;
    
    int fileNum = 0;
    char filename[10];
    for(;fileNum < numOfFile; fileNum++){
        sprintf(filename, "%06d", fileNum);
        cout << filename << endl;
    }
    fstream file;

    // connect to server
    int sockfd; 
    struct sockaddr_in servaddr; 
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    memset(&servaddr, 0, sizeof(servaddr));  
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(port); 
    servaddr.sin_addr.s_addr = inet_addr(address); 
    socklen_t len = sizeof(servaddr);
	if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        perror("connection"); 
        exit(EXIT_FAILURE); 
    } else {
		// printf("connect to server successfully\n");
    }
    
    return 0;
}