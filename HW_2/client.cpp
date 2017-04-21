//
// Created by smartjinyu on 4/21/17.
//

#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <memory.h>
#include <cstdio>
#include <unistd.h>

#define MAXLINE 4096

int max(int a, int b) {
    return a > b ? a : b;
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main(int argc, char **argv) {
    int sockfd;
    int maxfdp1, stdineof;
    fd_set rset;
    char sendline[MAXLINE] = {0}, recvline[MAXLINE] = {0};
    struct sockaddr_in serverAddr;
    if (argc != 3) {
        fputs("wrong arguments! Please run ./client.out <ip> <port> \n", stderr);
        return -1;
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((uint16_t) atoi(argv[2]));// port
    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);
    connect(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    FD_ZERO(&rset);
    stdineof = 0; // use for test readable, 0:connect

    while (true) {
        if (stdineof == 0) {
            FD_SET(fileno(stdin), &rset); // for input
        }
        FD_SET(sockfd, &rset); // for socket

        maxfdp1 = max(fileno(stdin), sockfd) + 1;
        select(maxfdp1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &rset)) {
            // socket is readable
            if (recv(sockfd, recvline, MAXLINE, 0) == 0) {
                printf("recv = 0\n");
                if (stdineof == 1) {
                    return 0; // normal termination
                } else {
                    printf("Client: Server terminated prematurely");
                    exit(-1);
                }
            }
            fputs(recvline, stdout);
            bzero(recvline, sizeof(recvline));
        }

        if (FD_ISSET(fileno(stdin), &rset)) {
            // input is readable

            if (read(fileno(stdin), sendline, MAXLINE) == 0) {
                //EOF
                stdineof = 1;
                shutdown(sockfd, SHUT_WR);// half close, close write connection here (client -> server)
                printf("half closed\n");
                FD_CLR(fileno(stdin), &rset);
                continue;
            }
            write(sockfd, sendline, strlen(sendline));
            bzero(sendline, sizeof(sendline));

        }


    }

}

#pragma clang diagnostic pop
