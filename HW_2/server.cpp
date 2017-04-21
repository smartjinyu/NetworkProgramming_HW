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
#include <errno.h>

#define MAXLINE 4096
#define MAXCLIENTS 128
#define LISTENQ 1024

struct clientInfo {
    char username[256] = {0};
    int sockfd = -1;
};


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main(int argc, char **argv) {
    int listenfd, connectfd, sockfd, maxClient, maxfd;
    // maxClient is max index of connected client
    int nready;
    clientInfo clients[MAXCLIENTS];
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
    maxClient = -1;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);


    while (true) {
        rset = allset;
        if ((nready = select(maxfd + 1, &rset, NULL, NULL, NULL)) < 0) {
            // number of ready descriptors

            if (errno == EINTR) {
                continue; // just ignore it
            } else {
                fprintf(stderr, "select error, error = %s\n", strerror(errno));
            }
        }
        if (FD_ISSET(listenfd, &rset)) {
            clientLen = sizeof(clientAddr);
            bzero(&clientAddr, sizeof(clientAddr));
            connectfd = accept(listenfd, (struct sockaddr *) &clientAddr, &clientLen);
            printf("New client connected, ip = %s, port = %d\n",
                   inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            // new client connection
            int i;
            for (i = 0; i < FD_SETSIZE; i++) {
                if (clients[i].sockfd < 0) {
                    // find the first available descriptor
                    clients[i].sockfd = connectfd;
                    break;
                }
            }
            if (i == FD_SETSIZE) {
                printf("too many clients\n");
            }
            FD_SET(connectfd, &allset);
            if (connectfd > maxfd) {
                maxfd = connectfd;
            }
            if (i > maxClient) {
                maxClient = i;
            }
            if (--nready <= 0) {
                continue;// no more readable descriptors
            }

        }

        for (int i = 0; i <= maxClient; i++) {
            // check all clients for data
            if ((sockfd = clients[i].sockfd) < 0) {
                continue; // skip empty client
            }
            if (FD_ISSET(sockfd, &rset)) {
                if ((n = read(sockfd, recvline, MAXLINE)) == 0) {
                    // connection closed by client
                    struct sockaddr_in terminatedAddr;
                    socklen_t len = sizeof(terminatedAddr);
                    getpeername(sockfd, (struct sockaddr *) &terminatedAddr, &len);
                    printf("Client terminated, ip = %s, port = %d\n",
                           inet_ntoa(terminatedAddr.sin_addr), ntohs(terminatedAddr.sin_port));

                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    clients[i].sockfd = -1;
                    continue;
                }

                fprintf(stdout, "Received from client: %s", recvline);
                if (clients[i].username[0] == 0) {
                    // username is not set
                    if (strncmp(recvline, "name:", 5) == 0) {
                        for (int j = 5; recvline[j] != '\n' && recvline[j] != 0; j++) {
                            clients[i].username[j - 5] = recvline[j];
                        }
                        printf("client's username is %s\n", clients[i].username);
                        write(sockfd, "Login successfully!\n", 20);
                    } else {
                        fputs("Username of the client not set, login failed!\n", stderr);
                        write(sockfd, "Username not set, login failed!\n", 32);
                        struct sockaddr_in terminatedAddr;
                        socklen_t len = sizeof(terminatedAddr);
                        getpeername(sockfd, (struct sockaddr *) &terminatedAddr, &len);
                        printf("Client terminated, ip = %s, port = %d\n",
                               inet_ntoa(terminatedAddr.sin_addr), ntohs(terminatedAddr.sin_port));
                        close(sockfd);
                        FD_CLR(sockfd, &allset);
                        clients[i].sockfd = -1;
                    }
                }

                bzero(&recvline, sizeof(recvline));

                
                if (--nready <= 0) {
                    break; // no more readable descriptors
                }

            }
        }
    }


}

#pragma clang diagnostic pop
