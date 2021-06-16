#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <ctime>
#include <fcntl.h>
using namespace std;

#define MYIP "10.1.0.177"
//#define MYIP "172.20.10.8"
#define SRVPORT "2500"
#define MSS 1000

typedef struct TCP_segment{
    unsigned int seq;
    unsigned int ack;
    bool URG, ACK, PSH, RST, SYN, FIN;
    char data[MSS];
}segment;

struct addrinfo serverinfo;
socklen_t server_addrlen;

segment file(int sockfd, segment send);

int main(){
    int sockfd, rv, numbyte;
    struct addrinfo temp, *srvinfo;// serverinfo;
    server_addrlen = sizeof(serverinfo);
    segment send, recv;
    srand(time(NULL));

    memset(&temp, 0, sizeof(temp));
    temp.ai_family = AF_INET;
    temp.ai_socktype = SOCK_DGRAM;

    if(getaddrinfo(MYIP, SRVPORT, &temp, &srvinfo)){
        perror("getaddrinfo");
        exit(1);
    }
    if((sockfd = socket(srvinfo->ai_family, srvinfo->ai_socktype, srvinfo->ai_protocol)) == -1){
        perror("Client socket");
        exit(1);
    }

    cout << "=====Start the three-way handshake=====" << endl;
    memset(&send, 0, sizeof(send));
    send.SYN=1;
    send.seq=rand()%10000;
    cout << "Send a packet(SYN) to " << MYIP << " :" << SRVPORT << endl;
    if((numbyte = sendto(sockfd, &send, sizeof(send), 0, srvinfo->ai_addr, srvinfo->ai_addrlen)) == -1 ){
        perror("Client sendto");
        exit(1);
    }
    //freeaddrinfo(srvinfo);
    memset(&serverinfo, 0, sizeof(serverinfo));
    memset(&recv, 0, sizeof(recv));
    if((numbyte = recvfrom(sockfd, &recv, sizeof(recv), 0, (struct sockaddr *)&serverinfo, &server_addrlen) == -1)){
        perror("Client recvfrom");
        exit(1);
    }
    cout << "Receive a packet(SYN/ACK) from " << MYIP << " :" << SRVPORT << endl;
    cout << "         Receive a packet (seq_num = " << recv.seq << ", ack_num = " << recv.ack << ")" << endl;
    cout << "Send a packet(ACK) to " << MYIP << " :" << SRVPORT << endl;
    memset(&send, 0, sizeof(send));
    send.SYN=0;
    send.seq=recv.ack;
    send.ack=recv.seq+1;
    if((numbyte = sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&serverinfo, sizeof(serverinfo))) == -1 ){
        perror("Client sendto");
        exit(1);
    }
    cout << "=====Complete the three-way handshake=====" << endl << endl;
    
    while(1){
        char buf[5], req[1000];
        char *r;
        string request, buf_tmp;
        cout << "Request (eg. MATH ADD 3 6,DNS google.com,FILE 1.mp4 or exit):" << endl;
        getline(cin, request);
        
        if(request=="exit")
            break;
        strcpy(req, request.c_str());
        //memset(&r, 0, sizeof(r));
        r = strtok(req, ",");         //MATH ADD 3 6
        while(r != NULL){
            memset(&send, 0, sizeof(send));
            send.seq=recv.ack;
            send.ack=recv.seq+1;
            strcpy(send.data, r);
            if((numbyte = sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&serverinfo, sizeof(serverinfo))) == -1 ){
                perror("Client request sendto");
                exit(1);
            }
            memset(&buf, 0, sizeof(buf));
            memset(&buf_tmp, 0, sizeof(buf_tmp));
            strncpy(buf, send.data, 4);
            buf[4]='\0';
            buf_tmp=buf;
            if(buf_tmp=="FILE"){
                cout << "Receive a file(Request) from " << MYIP << " :" << SRVPORT << endl;
                memset(&recv, 0, sizeof(recv));
                recv = file(sockfd, send);
            }
            else{
                memset(&recv, 0, sizeof(recv));
                if((numbyte = recvfrom(sockfd, &recv, sizeof(recv), 0, (struct sockaddr *)&serverinfo, &server_addrlen) == -1)){
                    perror("Client recvfrom");
                    exit(1);
                }
                cout << "Receive a file(Request) from " << MYIP << " :" << SRVPORT << endl;
                cout << "         Receive a packet (seq_num = " << recv.seq << ", ack_num = " << recv.ack << ")" << endl;
                cout << "\t" << recv.data << endl << endl;
            }

            r = strtok(NULL, ",");     //MATH SQUARE_ROOT 9,FILE 1.mp4,DNS google.com
        }
    }
}
//MATH SQUARE_ROOT 9
//MATH POWER 4 4
//MATH ADD 4 6
//MATH MINUS 6 7

segment file(int sockfd, segment send){
    segment recv;
    string filename;
    const char s[2] = " ";
    int i, seq_tmp, ack_tmp, numbyte, sum=0;
    seq_tmp = send.seq;
    ack_tmp = send.ack;
    for(i=5;i<=9;i++) filename+=send.data[i];
    // filename=strtok(send.data, s);
    // filename=strtok(NULL, s);
    filename="client_recv/"+filename;
    int file = open(filename.c_str(), O_WRONLY|O_TRUNC|O_CREAT, S_IRWXU);
    //receive filesize
    memset(&recv, 0, sizeof(recv));
    if((numbyte = recvfrom(sockfd, &recv, sizeof(recv), 0, (struct sockaddr *)&serverinfo, &server_addrlen) == -1)){
        perror("Client recvfrom");
        exit(1);
    }
    int size = recv.ack;  //size=total filesize
    //FILE first send!!!!
    memset(&send, 0, sizeof(send));
    send.seq=seq_tmp;
    send.ack=ack_tmp;
    if((numbyte = sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&serverinfo, sizeof(serverinfo))) == -1 ){
        perror("Client request sendto");
        exit(1);
    }
    while(size-sum>=MSS){
        memset(&recv, 0, sizeof(recv));
        if((numbyte = recvfrom(sockfd, &recv, sizeof(recv), 0, (struct sockaddr *)&serverinfo, &server_addrlen) == -1)){
            perror("Client recvfrom");
            exit(1);
        }
        cout << "         Receive a packet (seq_num = " << recv.seq << ", ack_num = " << recv.ack << ")" << endl;
        write(file, recv.data, MSS);
        sum += MSS;
        memset(&send, 0, sizeof(send));
        send.seq=recv.ack;
        send.ack=recv.seq+MSS;
        if((numbyte = sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&serverinfo, sizeof(serverinfo))) == -1 ){
            perror("Client request sendto");
            exit(1);
        }
    }
    memset(&recv, 0, sizeof(recv));
    if((numbyte = recvfrom(sockfd, &recv, sizeof(recv), 0, (struct sockaddr *)&serverinfo, &server_addrlen) == -1)){
        perror("Client recvfrom");
        exit(1);
    }
    cout << "         Receive a packet (seq_num = " << recv.seq << ", ack_num = " << recv.ack << ")" << endl;
    write(file, recv.data, size-sum);
    memset(&send, 0, sizeof(send));
    send.FIN=1;
    send.seq=recv.ack;
    send.ack=recv.seq+MSS;
    if((numbyte = sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&serverinfo, sizeof(serverinfo))) == -1 ){
        perror("Client request sendto");
        exit(1);
    }
    recv.ack+=1;
    recv.seq+=MSS;
    return recv;


}