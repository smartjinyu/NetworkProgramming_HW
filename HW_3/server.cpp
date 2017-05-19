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
#define MAXCLIENTS 128
#define MAXNAMELEN 256

struct clientInfo {
    char username[MAXNAMELEN] = {0};
    int sockfd = -1;
};

clientInfo clients[MAXCLIENTS];

void setReUseAddr(int sockfd) {
    int flag = 1;
    socklen_t optlen = sizeof(flag);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, optlen);
}


void *doit(void *);

void str_echo(int sockfd, int index) {
    ssize_t n;
    char recvline[MAXLINE];
    again:
    while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
        fputs(recvline, stdout);
        if (clients[index].username[0] == 0) {
            if (strncmp(recvline, "name:", 5) == 0) {
                // set username
                char name[256] = {0};
                strncpy(name, recvline + 5, strlen(recvline) - 6); // last character of recvline is \n
                int j = 0;
                for (j = 0; j <= MAXCLIENTS; j++) {
                    if (strcmp(clients[j].username, name) == 0) {
                        break;
                    }
                }
                if (j == MAXCLIENTS + 1) {
                    // this name is not used
                    strcpy(clients[index].username, name);
                    printf("client's username is %s\n", clients[index].username);
                    write(sockfd, "Login successfully!\n", 20);
                } else {
                    fputs("Username is already used, login failed!\n", stderr);
                    write(sockfd, "Username is already used, login failed!\n", 40);
                    return;
                }
            } else {
                fputs("Username of the client not set, login failed!\n", stderr);
                write(sockfd, "Username not set, login failed!\n", 32);
                return;
            }
        }

        bzero(recvline, sizeof(recvline));
    }
    if (n < 0 && errno == EINTR) {
        goto again; // ignore EINTR
    } else if (n < 0) {
        fprintf(stderr, "str_echo:read error");
        return;
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main(int argc, char **argv) {
    int listenfd, connfd;
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
        connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientAddrLen);
        int i = 0;
        for (i = 0; i < MAXCLIENTS; i++) {
            if (clients[i].sockfd == -1) {
                clients[i].sockfd = connfd;
                break;
            }
        }
        int *arg = (int *) malloc(sizeof(*arg));
        *arg = i;
        pthread_create(&tid, NULL, &doit, arg); /* pass by value*/
    }
}

#pragma clang diagnostic pop

void *doit(void *arg) {
    int index;
    index = *((int *) arg);
    free(arg);
    pthread_detach(pthread_self());
    int connfd = clients[index].sockfd;
    printf("New client connected, index = %d, sockfd = %d\n", index, connfd);
    str_echo(connfd, index);

    // thread terminated
    printf("Client %d terminated!\n", index);
    clients[index].sockfd = -1;
    bzero(clients[index].username, sizeof(clients[index].username));
    close(connfd);
    return (NULL);
}
