//
// Created by smartjinyu on 4/17/17.
//

#include <netinet/in.h>
#include <memory.h>
#include <arpa/inet.h>
#include <cstdio>

#define MAXLINE 4096

void dg_cli(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen) {
    int n;
    char sendline[MAXLINE], recvline[MAXLINE + 1];
    struct sockaddr* pcliaddr;
    socklen_t pclilen = sizeof(pcliaddr);
    bzero(&pcliaddr,sizeof(pcliaddr));

    while (fgets(sendline, MAXLINE, fp) != NULL) {
        sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);
        n = (int) recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
        recvline[n] = 0;
        fputs(recvline, stdout);
    }

}

int main() {
    int sockfd;
    socklen_t len;
    struct sockaddr_in servaddr,cliaddr;
    bzero(&servaddr, sizeof(servaddr));
    bzero(&cliaddr, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(9877);
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
    len = sizeof(cliaddr);
    getsockname(sockfd,(struct sockaddr *)&cliaddr,&len);
    printf("Client info, ip = %s, port =%d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
    dg_cli(stdin, sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    return 0;
}