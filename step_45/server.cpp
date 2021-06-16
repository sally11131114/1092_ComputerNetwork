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
#include <cmath>
#include <fstream>
#include <fcntl.h>
#include <random>
using namespace std;

#define SRVPORT 2500
#define MSS 1000

typedef struct TCP_segment{
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint16_t rwnd;
    uint16_t checksum;
    uint32_t options;
    bool URG, ACK, PSH, RST, SYN, FIN;
    char data[MSS];
}segment;

void find(int sockfd, segment recv);
void math(int sockfd, segment recv);
void dns(int sockfd, segment recv);
void file(int sockfd, segment recv);
struct addrinfo cliinfo;
socklen_t cli_addrlen;

int main(){
    int sockfd, rv, numbyte;
    struct addrinfo temp,  *srvinfo;
    cli_addrlen = sizeof(cliinfo);
    segment send, recv;
    pid_t pid;
    int port_temp=SRVPORT;
    srand(time(NULL));

    memset(&temp, 0, sizeof(temp));
    temp.ai_family = AF_INET;
    temp.ai_socktype = SOCK_DGRAM;
    temp.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, to_string(SRVPORT).c_str(), &temp, &srvinfo)){
        perror("getaddrinfo");
        exit(1);
    }
    if((sockfd = socket(srvinfo->ai_family, srvinfo->ai_socktype, srvinfo->ai_protocol)) == -1){
        perror("Server socket");
        exit(1);
    }
    if(bind(sockfd, srvinfo->ai_addr, srvinfo->ai_addrlen)==-1){
        close(sockfd);
        perror("Server bind");
        exit(1);
    }
    while(1){
        memset(&cliinfo, 0, sizeof(cliinfo));
        memset(&recv, 0, sizeof(recv));
        if((numbyte = recvfrom(sockfd, &recv, sizeof(recv), 0, (struct sockaddr *)&cliinfo, &cli_addrlen) == -1)){
            perror("Server recvfrom");
            exit(1);
        }
        port_temp++;
        pid = fork();
        //child process
        if(pid == 0){
            memset(&temp, 0, sizeof(temp));
            temp.ai_family = AF_INET;
            temp.ai_socktype = SOCK_DGRAM;
            temp.ai_flags = AI_PASSIVE;

            if(getaddrinfo(NULL, to_string(port_temp).c_str(), &temp, &srvinfo)){
                perror("getaddrinfo");
                exit(1);
            }
            if((sockfd = socket(srvinfo->ai_family, srvinfo->ai_socktype, srvinfo->ai_protocol)) == -1){
                perror("Server socket");
                exit(1);
            }
            if(bind(sockfd, srvinfo->ai_addr, srvinfo->ai_addrlen)==-1){
                close(sockfd);
                perror("Server bind");
                exit(1);
            }
            cout << "=====Start the three-way handshake=====" << endl;
            cout << "Receive a packet(SYN) from client" << endl;
            cout << "         Receive a packet (seq_num = " << recv.seq << ")" << endl;
            cout << "Send a packet(SYN/ACK) to client " << endl;
            memset(&send, 0, sizeof(send));
            send.SYN=1;
            send.seq=rand()%10000;
            send.ack=recv.seq+1;
            if((numbyte = sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&cliinfo, sizeof(cliinfo))) == -1 ){
                perror("Server sendto");
                exit(1);
            }
            memset(&recv, 0, sizeof(recv));
            if((numbyte = recvfrom(sockfd, &recv, sizeof(recv), 0, (struct sockaddr *)&cliinfo, &cli_addrlen) == -1)){
                perror("Server recvfrom");
                exit(1);
            }
            cout << "Receive a packet(ACK) from client" << endl;
            cout << "         Receive a packet (seq_num = " << recv.seq << ", ack_num = " << recv.ack << ")" << endl;
            cout << "=====Complete the three-way handshake=====" << endl << endl;

            while(1){
                memset(&recv, 0, sizeof(recv));
                if((numbyte = recvfrom(sockfd, &recv, sizeof(recv), 0, (struct sockaddr *)&cliinfo, &cli_addrlen) == -1)){
                    perror("Server recvfrom");
                    exit(1);
                }
                cout << "Receive a file(Request) from client" << endl;
                find(sockfd, recv);
            }
            return 0;
        }
    }
}
void find(int sockfd, segment recv){
    char buf[1000];
    string buf_tmp;
    const char s[2] = " ";
    strcpy(buf, recv.data);
    buf_tmp=strtok(buf, s);
    if(buf_tmp=="MATH") 
        math(sockfd, recv);
    else if(buf_tmp=="DNS")
        dns(sockfd,recv);
    else if(buf_tmp=="FILE")
        file(sockfd,recv);
    else
        cout << "find error" << endl;
}
void math(int sockfd, segment recv){
    char buf[1000];
    string buf_tmp, a, b, ans;
    int numbyte;
    const char s[2] = " ";
    strcpy(buf, recv.data);
    buf_tmp=strtok(buf, s); //buf_tmp = MATH
    buf_tmp=strtok(NULL, s); //buf_tmp= function

    if(buf_tmp=="ADD"){
        a=strtok(NULL, s);
        b=strtok(NULL, s);
        ans="The answer of "+buf_tmp+" "+a+" "+b+" is "+to_string(stof(a)+stof(b));
    }
    else if(buf_tmp=="MINUS"){
        a=strtok(NULL, s);
        b=strtok(NULL, s);
        ans="The answer of "+buf_tmp+" "+a+" "+b+" is "+to_string(stof(a)-stof(b));
    }
    else if(buf_tmp=="DIVIDE"){
        a=strtok(NULL, s);
        b=strtok(NULL, s);
        ans="The answer of "+buf_tmp+" "+a+" "+b+" is "+to_string(stof(a)/stof(b));
    }
    else if(buf_tmp=="MUTIPLE"){
        a=strtok(NULL, s);
        b=strtok(NULL, s);
        ans="The answer of "+buf_tmp+" "+a+" "+b+" is "+to_string(stof(a)*stof(b));
    }
    else if(buf_tmp=="SQUARE_ROOT"){
        a=strtok(NULL, s);
        ans="The answer of "+buf_tmp+" "+a+" is "+to_string(sqrt(stof(a)));
    }
    else if(buf_tmp=="POWER"){
        a=strtok(NULL, s);
        b=strtok(NULL, s);
        ans="The answer of "+buf_tmp+" "+a+" "+b+" is "+to_string(pow(stof(a), stof(b)));
    }
    cout << "         Receive a packet (seq_num = " << recv.seq << ", ack_num = " << recv.ack << ")" << endl;
    cout << "Send a packet(Request MATH) to client " << endl << endl;
    segment send;
    memset(&send, 0, sizeof(send));
    send.seq=recv.ack;
    send.ack=recv.seq+1;
    strcpy(send.data, ans.c_str());
    if((numbyte = sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&cliinfo, sizeof(cliinfo))) == -1 ){
        perror("Server request(MATH) sendto");
        exit(1);
    }
}
void dns(int sockfd, segment recv){
    char buf[1000];
    string buf_tmp, ans_tmp;
    char ans[INET6_ADDRSTRLEN];
    int numbyte;
    const char s[2] = " ";
    strcpy(buf, recv.data);
    buf_tmp=strtok(buf, s); //buf_tmp = DNS
    buf_tmp=strtok(NULL, s); //buf_tmp= "google.com"

    struct addrinfo temp, *res;
    memset(&temp, 0, sizeof(temp));
    temp.ai_family=AF_INET;
    temp.ai_socktype=SOCK_STREAM;

    if(getaddrinfo(buf_tmp.c_str(), NULL, &temp, &res)){
        perror("DNS getaddrinfo");
        exit(1);
    }
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(res->ai_family, &(ipv4->sin_addr), ans, sizeof ans);
    ans_tmp="The ipv4 of "+buf_tmp+" is "+ans;
    cout << "         Receive a packet (seq_num = " << recv.seq << ", ack_num = " << recv.ack << ")" << endl;
    cout << "Send a packet(Request DNS) to client " << endl << endl;
    segment send;
    memset(&send, 0, sizeof(send));
    send.seq=recv.ack;
    send.ack=recv.seq+1;
    strcpy(send.data, ans_tmp.c_str());
    if((numbyte = sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&cliinfo, sizeof(cliinfo))) == -1 ){
        perror("Server request(DNS) sendto");
        exit(1);
    }
}
void file(int sockfd, segment recv){
    //loss packet
    default_random_engine generator;
    bernoulli_distribution distribution(0.00001);

    char buf[1000];
    string buf_tmp, filename;
    segment send, send2;
    int numbyte, size_tmp=0;
    const char s[2] = " ";
    strcpy(buf, recv.data);
    buf_tmp=strtok(buf, s); //buf_tmp = FILE
    filename=strtok(NULL, s); //buf_tmp= 1.mp4
    //filesize
    ifstream f(filename, ios::binary);
    f.seekg(0, ios::end);
    int size = f.tellg();
    //send filesize
    memset(&send, 0, sizeof(send));
    send.ack=size;
    if((numbyte = sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&cliinfo, sizeof(cliinfo))) == -1 ){
        perror("Server request sendto");
        exit(1);
    }
    //sendfile
    int file = open(filename.c_str(),O_RDONLY);
    while(1){
        //server first receive
        memset(&recv, 0, sizeof(recv));
        if((numbyte = recvfrom(sockfd, &recv, sizeof(recv), 0, (struct sockaddr *)&cliinfo, &cli_addrlen) == -1)){
            perror("Server recvfrom");
            exit(1);
        }
        cout << "         Receive a packet(ACK) (ack_num = " << recv.ack << ")" << endl;
        
        if(recv.FIN == 1) break;
        lseek(file, size_tmp, SEEK_SET);
        //recv.ack+=send.seq;
        //server first send
        int check=1;
        //1
        memset(&send, 0, sizeof(send));
        send.seq=recv.ack;
        send.ack=recv.seq+MSS;
        read(file, send.data, MSS);
        if(distribution(generator)){
            cout<<"Server : loss packet !!"<<endl;
            send.checksum=1;
            sleep(1);
            check=0;
        }
        if((numbyte = sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&cliinfo, sizeof(cliinfo))) == -1 ){
            perror("Server request sendto");
            exit(1);
        }cout << "Send a packet(ACK) (seq_num = " << send.seq << ")" << endl;

        //2                     
        memset(&send2, 0, sizeof(send2));
        send2.seq=send.seq+MSS;
        send2.ack=send.ack;
        read(file, send2.data, MSS);
        if(distribution(generator)){
            cout<<"Server : loss packet !!"<<endl;
            send2.checksum=1;
            sleep(1);
            check=0;
        }
        if((numbyte = sendto(sockfd, &send2, sizeof(send2), 0, (struct sockaddr *)&cliinfo, sizeof(cliinfo))) == -1 ){
            perror("Server request sendto");
            exit(1);
        }cout << "Send a packet(ACK) (seq_num = " << send2.seq << ")" << endl;
        if(check) size_tmp=size_tmp+MSS*2;
    }
                                
}