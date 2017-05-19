//
// Created by smartjinyu on 5/19/17.
//

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <pthread.h>
#include <errno.h>

#define MAXLINE 4096
#define LISTENQ 1024

void setReUseAddr(int sockfd) {
    int flag = 1;
    socklen_t optlen = sizeof(flag);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, optlen);
}


void *doit(void *);

void str_echo(int sockfd) {
    ssize_t n;
    char buf[MAXLINE];
    again:
    while ((n = read(sockfd, buf, MAXLINE)) > 0) {
        write(sockfd, buf, n);
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
    int listenfd, *iptr;
    pthread_t tid;
    socklen_t clientAddrLen;
    struct sockaddr_in servaddr, clientaddr;


    if (argc != 2) {
        fprintf(stderr, "Wrong arguments! Please run ./Server <port>\n");
        exit(-1);
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    setReUseAddr(listenfd);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((uint16_t) atoi(argv[1]));

    bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    listen(listenfd, LISTENQ);

    for (;;) {
        bzero(&clientaddr, sizeof(clientaddr));
        clientAddrLen = sizeof(clientaddr);
        iptr = (int *) malloc(sizeof(int)); /* for each thread */
        *iptr = accept(listenfd, (struct sockaddr *) &clientaddr, &clientAddrLen);

        pthread_create(&tid, NULL, &doit, iptr); /* pass by value*/
    }
}
#pragma clang diagnostic pop

void *doit(void *arg) {
    int connfd;
    connfd = *((int *) arg);
    free(arg);
    pthread_detach(pthread_self());
    str_echo(connfd);
    close(connfd);
    return (NULL);
}
