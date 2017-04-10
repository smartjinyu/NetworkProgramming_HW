//
// Created by smartjinyu on 3/20/17.
//


#include <netinet/in.h>
#include <strings.h>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>


#define MAXLINE 4096
#define LISTENQ 1024


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
int main(int argc, char **argv) {

    int listenid, connfd, sockfd, i, maxi, maxfd;
    int nready, client[FD_SETSIZE];
    fd_set rset, allset;
    ssize_t n;
    char line[MAXLINE];
    //pid_t childpid;
    struct sockaddr_in servaddr, clientaddr;
    socklen_t clientlen;

    listenid = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);// for any interface on the server
    servaddr.sin_port = htons(9877);// port 9877

    bind(listenid, (struct sockaddr *) &servaddr, sizeof(servaddr));
    // use bind to monitor the request on the port
    listen(listenid, LISTENQ);
    maxfd = listenid; // initialize maxfd
    maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++) {
        client[i] = -1; // -1 indicates available entry
    }
    FD_ZERO(&allset);
    FD_SET(listenid, &allset);
    while (true) {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        // number of ready descriptors
        if (FD_ISSET(listenid, &rset)) {
            clientlen = sizeof(clientaddr);
            connfd = accept(listenid, (struct sockaddr *) &clientaddr, &clientlen);
            // new client connection
            for (i = 0; i < FD_SETSIZE; i++) {
                if (client[i] < 0) { // find first available descriptor
                    client[i] = connfd;// save descriptor
                    break;
                }
            }
            if (i == FD_SETSIZE) {
                printf("too many clients");
            }
            FD_SET(connfd, &allset);// add new descriptor to set
            if (connfd > maxfd) {
                maxfd = connfd;
            }
            if (i > maxi) {
                maxi = i;
            }

            if (--nready <= 0) {
                continue; // no more readable descriptors
            }
        }

        for (i = 0; i <= maxi; i++) {
            // check all clients for data
            if ((sockfd = client[i]) < 0) {
                continue; // skip empty client
            }
            if (FD_ISSET(sockfd, &rset)) {
                if ((n = read(sockfd, line, MAXLINE)) == 0) {
                    // connection closed by client
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                } else {
                    write(sockfd, line, (size_t)n);
                    fputs(line,stdout);
                    for(int j=0;j<sizeof(line);j++){
                        line[j]=0;
                    }
                }
            }
            if (--nready < 0) {
                break; // no more readable descriptors
            }
        }
    }
}
#pragma clang diagnostic pop

