//
// Created by smartjinyu on 4/24/17.
//

#include <iostream>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define MAXLINE 4096

void str_cli(FILE *fp, int sockfd) {
    char sendline[MAXLINE] = {0}, recvline[MAXLINE] = {0};
    while (fgets(sendline, MAXLINE, fp) != NULL) {
        write(sockfd, sendline, strlen(sendline));
        if (read(sockfd, recvline, MAXLINE) == 0) {
            printf("str_cli: server terminated prematurely");
            exit(0);
        }
        fputs(recvline, stdout);
        bzero(recvline, sizeof(recvline));
        bzero(sendline, sizeof(sendline));
    }
}

void dg_cli(FILE *fp,int sockfd,const struct sockaddr * pseraddr,socklen_t serlen){
    int n;
    char sendline[MAXLINE]={0},recvline[MAXLINE+1]={0};

    while(fgets(sendline,MAXLINE,fp)!=NULL){
        sendto(sockfd, sendline, strlen(sendline), 0, pseraddr, serlen);
        n = (int) recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
        recvline[n] = 0;
        fputs(recvline, stdout);
        bzero(recvline, sizeof(recvline));
        bzero(sendline, sizeof(sendline));

    }
}


void tcp(char * ip,uint16_t port){
    int sockfd;
    //char recvline[MAXLINE + 1];
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // the third parameter Specifying a protocol of 0 causes socket() to use an unspecified default
    // protocol appropriate for the requested socket type (e.g., TCP/IP)
    // create a socket


    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port); // use port 9877
    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
        // translate the ip address
        printf("inet_pton error for %s",ip);
        return;
    }

    connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    str_cli(stdin, sockfd);

}

void udp(char * ip,uint16_t port) {
    int sockfd;
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &servaddr.sin_addr);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    dg_cli(stdin,sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
}


int main(int argc, char **argv) {

    if(argc!=4){
        fprintf(stderr,"wrong arguments, please run ./client <TCP/UDP> <ip> <port>\n");
    }
    if(strncmp(argv[1],"TCP",3)==0){
        tcp(argv[2],(uint16_t) atoi(argv[3]));
    }else if(strncmp(argv[1],"UDP",3)==0){
        udp(argv[2],(uint16_t) atoi(argv[3]));
    }else{
        fprintf(stderr,"wrong arguments, please run ./client <TCP/UDP> <ip> <port>\n");
    }

    return 0;
}
