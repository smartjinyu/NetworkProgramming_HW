
//
// Created by smartjinyu on 3/20/17.
//


#include <iostream>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define MAXLINE 4096

void str_cli(FILE *fp, int sockfd) {
    char sendline[MAXLINE], recvline[MAXLINE];
    while (fgets(sendline, MAXLINE, fp) != NULL) {
        write(sockfd, sendline, strlen(sendline));
        if (read(sockfd, recvline, MAXLINE) == 0) {
            printf("str_cli: server terminated prematurely");
            exit(0);
        }
        fputs(recvline, stdout);
    }
}

int main(int argc, char **argv) {


    int sockfd;
    //char recvline[MAXLINE + 1];
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // the third parameter Specifying a protocol of 0 causes socket() to use an unspecified default
    // protocol appropriate for the requested socket type (e.g., TCP/IP)
    // create a socket



    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(9877); // use port 9877
    if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0) {
        // translate the ip address
        printf("inet_pton error for %s", "127.0.0.1");
        return -1;
    }
    if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0){
        printf("Connect failed, error message = %s\n",strerror(errno));
    }else{
        printf("Connect successfully!\n");
    }
    str_cli(stdin, sockfd);


    return 0;
}