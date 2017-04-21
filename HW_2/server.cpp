//
// Created by smartjinyu on 4/21/17.
//

#include <sys/param.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <memory.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXLINE 4096
#define LISTENQ 1024

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main(int argc, char **argv) {
    int listenfd, connectfd, sockfd, maxi, maxfd;
    int nready, client[FD_SETSIZE];
    fd_set rset, allset;
    ssize_t n;
    char recvline[MAXLINE] = {0};
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen;

    if (argc != 2) {
        fputs("Wrong arguments! Please run ./server.out <port> \n", stderr);
        exit(-1);
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons((uint16_t) atoi(argv[1]));

    bind(listenfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    // monitor the request on the port

    listen(listenfd, LISTENQ);
    // LISTENQ the max length of the backlog (= complete + incomplete request queue)

    maxfd = listenfd;
    maxi = -1;
    for (int i = 0; i < FD_SETSIZE; i++) {
        client[i] = -1;
    }
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    while (true) {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        // number of ready descriptors
        if (FD_ISSET(listenfd, &rset)) {
            clientLen = sizeof(clientAddr);
            bzero(&clientAddr, sizeof(clientAddr));
            connectfd = accept(listenfd, (struct sockaddr *) &clientAddr, &clientLen);
            printf("New client connected, ip = %s, port = %d\n",
                   inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            // new client connection
            int i;
            for (i = 0; i < FD_SETSIZE; i++) {
                if (client[i] < 0) {
                    // find the first available descriptor
                    client[i] = connectfd;
                    break;
                }
            }
            if (i == FD_SETSIZE) {
                printf("Too many clients\n");
            }
            FD_SET(connectfd, &allset);
            if (connectfd > maxfd) {
                maxfd = connectfd;
            }
            if (i > maxi) {
                maxi = i;
            }
            if (--nready <= 0) {
                continue;// no more readable descriptors
            }
        }


        for (int i = 0; i <= maxi; i++) {
            // check all clients for data
            if ((sockfd = client[i] < 0)) {
                continue; // skip empty client
            }
            if (FD_ISSET(sockfd, &rset)) {
                bzero(&recvline, sizeof(recvline));
                if ((n = read(sockfd, recvline, MAXLINE)) == 0) {
                    // connection closed by client
                    struct sockaddr_in terminatedAddr;
                    socklen_t len = sizeof(terminatedAddr);
                    getpeername(sockfd, (struct sockaddr *) &terminatedAddr, &len);
                    printf("Client terminated, ip = %s, port = %d\n",
                           inet_ntoa(terminatedAddr.sin_addr), ntohs(terminatedAddr.sin_port));

                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                } else {
                    fputs(recvline, stdout);

                    //todo

                    write(sockfd, recvline, (size_t) n);

                }
                if (--nready <= 0) {
                    break; // no more readable descriptors
                }

            }
        }

    }


}

#pragma clang diagnostic pop
