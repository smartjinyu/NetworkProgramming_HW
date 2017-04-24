//
// Created by smartjinyu on 4/24/17.
//

#include <unistd.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <wait.h>
#include <cstdio>
#include <memory.h>
#include <errno.h>
#include <cstdlib>
#include <arpa/inet.h>

#define MAXLINE 4096
#define LISTENQ 1024

void sig_child_waitpid(int signal) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        printf("child %d terminated\n", pid);

    }
    return;
}

int max(int a, int b) {
    return a > b ? a : b;
}

void str_echo(int sockfd) {
    ssize_t n;
    char buf[MAXLINE] = {0};
    again:
    while ((n = read(sockfd, buf, MAXLINE)) > 0) {
        printf("Receives: %s",buf);
        write(sockfd, buf, n);
        bzero(buf, sizeof(buf));
    }
    if (n < 0 && errno == EINTR) {
        goto again; // ignore EINTR
    } else if (n < 0) {
        printf("str_echo:read error");
    }
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main(int argc, char **argv) {
    int listenfd, connfd, udpfd, nready, maxfdp1;
    char mesg[MAXLINE] = {0};
    pid_t childpid;
    fd_set rset;
    ssize_t n;
    socklen_t len;
    const int on = 1;
    struct sockaddr_in cliaddr, servaddr;

    if (argc != 2) {
        fprintf(stderr, "wrong arguments, please run ./server <port>\n");
    }



    /* for create listening TCP socket */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((uint16_t) atoi(argv[1]));

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    listen(listenfd, LISTENQ);

    /* for creating UDP socket */
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((uint16_t) atoi(argv[1]));

    bind(udpfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    signal(SIGCHLD, sig_child_waitpid);

    FD_ZERO(&rset);
    maxfdp1 = max(listenfd, udpfd) + 1;

    while (true) {
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);
        if ((nready = select(maxfdp1, &rset, NULL, NULL, NULL)) < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                fprintf(stderr, "select erro, error = %s\n", strerror(errno));
            }
        }
        if (FD_ISSET(listenfd, &rset)) {
            len = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len);
            if ((childpid = fork()) == 0) {
                printf("New client connected from TCP, server = %s,port = %d\n",
                       inet_ntoa(cliaddr.sin_addr),
                       (int) ntohs(cliaddr.sin_port));

                close(listenfd);
                str_echo(connfd);
                exit(0);
            }
            close(connfd);
        }

        if (FD_ISSET(udpfd, &rset)) {
            len = sizeof(cliaddr);
            bzero(&cliaddr, sizeof(cliaddr));
            n = recvfrom(udpfd, mesg, MAXLINE, 0, (struct sockaddr *) &cliaddr, &len);
            sendto(udpfd, mesg, n, 0, (struct sockaddr *) &cliaddr, len);
            printf("Received from client using UDP, ip = %s, port = %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
            printf("Receives: %s",mesg);

            bzero(mesg, sizeof(mesg));
        }
    }
    return 0;
}

#pragma clang diagnostic pop