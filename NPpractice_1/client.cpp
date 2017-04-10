//
// Created by smartjinyu on 3/13/17.
//

#include <iostream>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXLINE 4096

int main(int argc, char **argv) {
    int sockfd, n;
    char recvline[MAXLINE + 1];
    struct sockaddr_in servaddr;
    if (argc != 3) {
        printf("Wrong args. Usage: a.out <IP Address> <Port>");
        return -1;
    }
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // the third parameter Specifying a protocol of 0 causes socket() to use an unspecified default
        // protocol appropriate for the requested socket type (e.g., TCP/IP)
        printf("socket error %d \n",sockfd);
        return -1;
    }
    // create a socket

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons((uint16_t)atoi(argv[2])); // use port 13
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        // translate the ip address
        printf("inet_pton error for %s", argv[1]);
        return -1;
    }
    int j;
    if ((j = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0) {
        // establish connection using TCP protocol
        printf("connection error %d\n",j);
        return -1;
    }
    while ((n = (int) read(sockfd, recvline, MAXLINE)) > 0) {
        // read() returns n of bytes received
        // if returns 0, connection has been gracefully closed
        // if returns x < 0, something went wrong
        recvline[n] = 0;
        if (fputs(recvline, stdout) == EOF) {
            // fputs() returns non-negative value if succeed
            // otherwise returns EOF
            // stdout is file pointer for standard output stream
            printf("fputs error");
        }
    }
    if (n < 0) {
        printf("read error");
        return -1;
    }

    return 0;
}