//
// Created by smartjinyu on 4/17/17.
//

#include <netinet/in.h>
#include <memory.h>
#include <cstdio>
#include <arpa/inet.h>

#define MAXLINE 4096
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clientlen) {
    int n;
    socklen_t len;
    char message[MAXLINE];
    struct sockaddr_in* cliaddr;
    while (1) {
        len = clientlen;
        n = (int) recvfrom(sockfd, message, MAXLINE, 0, pcliaddr, &len);
        bzero(&cliaddr,sizeof(cliaddr));
        cliaddr = (struct sockaddr_in*) pcliaddr;
        printf("Received from client, ip = %s, port = %d\n",inet_ntoa(cliaddr->sin_addr),ntohs(cliaddr->sin_port));
        sendto(sockfd, message, n, 0, pcliaddr, len);
    }
}

#pragma clang diagnostic pop


int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = (INADDR_ANY);
    servaddr.sin_port = htons(9877);

    bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    dg_echo(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}