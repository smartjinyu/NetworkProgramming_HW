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
#include <vector>

#define MAXLINE 4096
#define MAXCLIENTS 128
#define LISTENQ 1024
#define POSTSAVEFILE "posts"


struct clientInfo {
    char username[256] = {0};
    int sockfd = -1;
};

struct article {
    char content[MAXLINE] = {0};
    char username[256] = {0};
    char ipAddr[256] = {0};
    int port;

    void printArticle() {
        printf("Post:\n");
        printf("Client username = %s, ip address = %s, port = %d\n", username, ipAddr, port);
        printf("Content: %s\n", content);
    }

    void sendArticleToClient(int sockfd) {
        write(sockfd, "Post:\n", 6);
        char sendline[MAXLINE] = {0};
        sprintf(sendline, "Client username = %s, ip address = %s, port = %d\n", username, ipAddr, port);
        write(sockfd, sendline, strlen(sendline));
        bzero(sendline, sizeof(sendline));
        sprintf(sendline, "Content: %s\n", content);
        write(sockfd, sendline, strlen(sendline));
    }
};

std::vector<article> articles; // vector of articles posted till now

void loadArticleFromDisk() {
    // read articles from disk, call on start
    article mArticle;
    FILE *file = fopen(POSTSAVEFILE, "rb");
    if (file != NULL) {

        while (fread(&mArticle, sizeof(struct article), 1, file) != 0) {
            articles.push_back(mArticle);
        }
        fclose(file);
    } else {
        fprintf(stderr, "Failed to open the file to read articles, error = %s", strerror(errno));
    }

}

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
    const int socketOpOn = 1;


    if (argc != 2) {
        fputs("Wrong arguments! Please run ./server.out <port> \n", stderr);
        exit(-1);
    }

    loadArticleFromDisk();

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons((uint16_t) atoi(argv[1]));

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &socketOpOn, sizeof(socketOpOn));

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
                    bzero(clients[i].username, sizeof(clients[i].username));
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
                    bzero(&terminatedAddr, sizeof(terminatedAddr));
                    socklen_t len = sizeof(terminatedAddr);
                    getpeername(sockfd, (struct sockaddr *) &terminatedAddr, &len);
                    printf("Client terminated, ip = %s, port = %d\n",
                           inet_ntoa(terminatedAddr.sin_addr), ntohs(terminatedAddr.sin_port));

                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    clients[i].sockfd = -1;
                    bzero(clients[i].username, sizeof(clients[i].username));
                    continue;
                }

                fprintf(stdout, "Received: %s", recvline);
                if (clients[i].username[0] == 0) {
                    // username is not set
                    if (strncmp(recvline, "name:", 5) == 0) {
                        // set username
                        strncpy(clients[i].username, recvline + 5, strlen(recvline) - 6);
                        // last character of recvline is \n
                        printf("client's username is %s\n", clients[i].username);
                        write(sockfd, "Login successfully!\n", 20);
                    } else {
                        fputs("Username of the client not set, login failed!\n", stderr);
                        write(sockfd, "Username not set, login failed!\n", 32);
                        struct sockaddr_in terminatedAddr;
                        bzero(&terminatedAddr, sizeof(terminatedAddr));
                        socklen_t len = sizeof(terminatedAddr);
                        getpeername(sockfd, (struct sockaddr *) &terminatedAddr, &len);
                        printf("Client terminated, ip = %s, port = %d\n",
                               inet_ntoa(terminatedAddr.sin_addr), ntohs(terminatedAddr.sin_port));
                        close(sockfd);
                        FD_CLR(sockfd, &allset);
                        clients[i].sockfd = -1;
                        bzero(clients[i].username, sizeof(clients[i].username));
                    }

                }

                if (strncmp(recvline, "post:", 5) == 0) {
                    // post an article
                    struct sockaddr_in curClientAddr;
                    bzero(&curClientAddr, sizeof(curClientAddr));
                    socklen_t len = sizeof(curClientAddr);
                    getpeername(sockfd, (struct sockaddr *) &curClientAddr, &len);
                    article mArticle;
                    strcpy(mArticle.username, clients[i].username);
                    strcpy(mArticle.ipAddr, inet_ntoa(curClientAddr.sin_addr));
                    mArticle.port = ntohs(curClientAddr.sin_port);
                    strncpy(mArticle.content, recvline + 5, strlen(recvline) - 6);

                    FILE *file = fopen(POSTSAVEFILE, "a+b");
                    if (file != NULL) {
                        fwrite(&mArticle, sizeof(struct article), 1, file);
                        fclose(file);
                        printf("New article posted:\n");
                        mArticle.printArticle();
                        write(sockfd, "Post new article successfully!\n", 31);
                        articles.push_back(mArticle);
                    } else {
                        fprintf(stderr, "Failed to save new article to file, error = %s", strerror(errno));
                        write(sockfd, "Failed to post the article!\n", 28);
                    }
                } else if (strncmp(recvline, "listposts", 9) == 0) {
                    // list all the posts stored
                    if (articles.size() == 0) {
                        write(sockfd, "No posts currently!\n", 20);
                    } else {
                        char sendline[MAXLINE] = {0};
                        sprintf(sendline, "Listing all the %ld posts catalog\n", articles.size());
                        write(sockfd, sendline, strlen(sendline));
                        bzero(sendline, sizeof(sendline));
                        for (int j = 0; j < articles.size(); j++) {
                            sprintf(sendline, "Post #%d\nClient username = %s, ip address = %s, port = %d\n", j,
                                    articles[j].username, articles[j].ipAddr, articles[j].port);
                            write(sockfd, sendline, strlen(sendline));
                            bzero(sendline, sizeof(sendline));
                        }
                    }

                } else if (strncmp(recvline, "readpost:", 9) == 0) {
                    char buff[256] = {0};
                    strncpy(buff, recvline + 9, strlen(recvline) - 10);
                    int index = atoi(buff);
                    if (index >= articles.size()) {
                        bzero(buff, sizeof(buff));
                        sprintf(buff, "Wrong post index %d, max valid post index is %ld\n", index, articles.size() - 1);
                        fprintf(stderr, buff);
                        write(sockfd, buff, strlen(buff));
                    } else {
                        articles[index].sendArticleToClient(sockfd);
                    }

                } else if (strncmp(recvline, "listclients", 11) == 0) {
                    // list all the clients online
                    int clientSocketfd;
                    char sendline[256] = {0};
                    for (int j = 0; j <= maxClient; j++) {
                        if ((clientSocketfd = clients[j].sockfd) != -1 && clients[j].username[0] != 0) {
                            struct sockaddr_in curClientAddr;
                            bzero(&curClientAddr, sizeof(curClientAddr));
                            socklen_t len = sizeof(curClientAddr);
                            getpeername(clientSocketfd, (struct sockaddr *) &curClientAddr, &len);
                            sprintf(sendline, "Client name = %s, ip = %s, port = %d\n", clients[j].username,
                                    inet_ntoa(curClientAddr.sin_addr), ntohs(curClientAddr.sin_port));
                            write(sockfd, sendline, strlen(sendline));

                        }
                    }
                } else if (strncmp(recvline, "broadcast:", 10) == 0) {
                    char content[MAXLINE] = {0};
                    sprintf(content, "Broadcast from client %s:", clients[i].username);
                    strncat(content, recvline + 10, strlen(recvline) - 10);
                    // keep \n
                    int udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
                    struct sockaddr_in udpaddr;
                    struct sockaddr_in curClientAddr;
                    socklen_t len = sizeof(curClientAddr);
                    int clientSocketfd;
                    for (int j = 0; j <= maxClient; j++) {
                        if ((clientSocketfd = clients[j].sockfd) != -1 && clients[j].username[0] != 0) {
                            bzero(&curClientAddr, sizeof(curClientAddr));
                            getpeername(clientSocketfd, (struct sockaddr *) &curClientAddr, &len);
                            bzero(&udpaddr, sizeof(udpaddr));
                            udpaddr.sin_family = AF_INET;
                            udpaddr.sin_port = curClientAddr.sin_port;
                            udpaddr.sin_addr = curClientAddr.sin_addr;
                            len = sizeof(udpaddr);
                            sendto(udpsockfd, content, strlen(content), 0, (struct sockaddr *) &udpaddr, len);
                        }
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
