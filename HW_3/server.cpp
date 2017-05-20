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
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <dirent.h>

#define MAXLINE 4096
#define LISTENQ 1024
#define MAXCLIENTS 128
#define MAXNAMELEN 256

struct clientInfo {
    char username[MAXNAMELEN] = {0};
    int sockfd = -1;
    std::vector<std::string> filenames;
};

clientInfo clients[MAXCLIENTS];
std::vector<std::string> serverFiles;

void setReUseAddr(int sockfd) {
    int flag = 1;
    socklen_t optlen = sizeof(flag);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, optlen);
}

void listfiles() {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(".")) != NULL) {
        /* print all the files and directories within directory */
        serverFiles.clear();
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                // regular file
                serverFiles.push_back(ent->d_name);
            }
        }
        closedir(dir);
    } else {
        /* could not open directory */
        perror("");
        return;
    }

}

void showClientTerminatedInfo(int sockfd) {
    struct sockaddr_in terminatedAddr;
    bzero(&terminatedAddr, sizeof(terminatedAddr));
    socklen_t len = sizeof(terminatedAddr);
    getpeername(sockfd, (struct sockaddr *) &terminatedAddr, &len);
    printf("Client terminated, ip = %s, port = %d\n",
           inet_ntoa(terminatedAddr.sin_addr), ntohs(terminatedAddr.sin_port));
}

void *doit(void *);

void str_echo(int index) {
    ssize_t n;
    char recvline[MAXLINE] = {0};
    int sockfd = clients[index].sockfd;
    again:
    while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
        fputs(recvline, stdout);
        if (strncmp(recvline, "listMyfiles:<", 13) == 0) {
            // get file info in client
            printf("\n");
            char buff[MAXLINE] = {0};
            strncpy(buff, recvline, strlen(recvline));
            while (recvline[strlen(recvline) - 1] != '>') {
                bzero(recvline, sizeof(recvline));
                read(sockfd, recvline, MAXLINE);
                strncat(buff, recvline, strlen(recvline));
            }
            // ensure the data is complete
            int i = 12; // 10 is the character <, i means the previous ','
            int j = 12; // j means the current ','
            char name[MAXNAMELEN] = {0};
            clients[index].filenames.clear();
            for (int k = 12; buff[k] != '>'; k++) {
                if (buff[k] == ',') {
                    i = j;
                    j = k;
                    strncpy(name, buff + i + 1, (size_t) (j - i - 1));
                    clients[index].filenames.push_back(name);
                    bzero(name, sizeof(name));
                }
            }
            bzero(recvline, sizeof(recvline));
            continue;
        }


        if (clients[index].username[0] == 0) {
            if (strncmp(recvline, "name:", 5) == 0) {
                // set username
                char name[256] = {0};
                strncpy(name, recvline + 5, strlen(recvline) - 6); // last character of recvline is \n
                int j = 0;
                for (j = 0; j < MAXCLIENTS; j++) {
                    if (strcmp(clients[j].username, name) == 0) {
                        break;
                    }
                }
                if (j == MAXCLIENTS) {
                    // this name is not used
                    strcpy(clients[index].username, name);
                    printf("client's username is %s\n", clients[index].username);
                    write(sockfd, "Login successfully!\n", 20);
                    bzero(recvline, sizeof(recvline));
                    continue;
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
        if (strncmp(recvline, "listclients", 11) == 0) {
            // list all clients online
            int clientSocketfd;
            char sendline[256] = {0};
            for (int j = 0; j < MAXCLIENTS; j++) {
                if ((clientSocketfd = clients[j].sockfd) != -1 && clients[j].username[0] != 0) {
                    struct sockaddr_in curClientAddr;
                    bzero(&curClientAddr, sizeof(curClientAddr));
                    socklen_t len = sizeof(curClientAddr);
                    getpeername(clientSocketfd, (struct sockaddr *) &curClientAddr, &len);
                    sprintf(sendline, "Client index = %d,name = %s, ip = %s, port = %d\n", j, clients[j].username,
                            inet_ntoa(curClientAddr.sin_addr), ntohs(curClientAddr.sin_port));
                    write(sockfd, sendline, strlen(sendline));
                    bzero(sendline, sizeof(sendline));
                }
            }
        }

        if (strncmp(recvline, "listfiles:", 10) == 0) {
            // list files of the client
            char buff[MAXNAMELEN] = {0};
            strncpy(buff, recvline + 10, strlen(recvline) - 11); // eliminate \n
            int i = atoi(buff);
            char sendline[MAXLINE] = {0};
            if (i == -1) {
                sprintf(sendline, "Listing files in server\n");
                write(sockfd, sendline, strlen(sendline));
                for (int j = 0; j < serverFiles.size(); j++) {
                    bzero(sendline, sizeof(sendline));
                    strcpy(sendline, serverFiles[j].c_str());
                    strcat(sendline, "\n");
                    write(sockfd, sendline, strlen(sendline));
                }
            } else {
                if (0 <= i < MAXCLIENTS && clients[i].sockfd != -1) {
                    sprintf(sendline, "Listing files in client %s\n", clients[i].username);
                    write(sockfd, sendline, strlen(sendline));
                    for (int j = 0; j < clients[i].filenames.size(); j++) {
                        bzero(sendline, sizeof(sendline));
                        strcpy(sendline, clients[i].filenames[j].c_str());
                        strcat(sendline, "\n");
                        write(sockfd, sendline, strlen(sendline));
                    }
                } else {
                    sprintf(sendline, "The client index %d is not valid!\n", i);
                    fprintf(stderr, sendline);
                    write(sockfd, sendline, strlen(sendline));

                }
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
    listfiles();

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
        printf("New client connected, index = %d, ip = %s, port = %d\n",
               i, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

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
    // printf("New client connected, index = %d, sockfdClient = %d\n", index, connfd);
    str_echo(index);

    // thread terminated
    showClientTerminatedInfo(clients[index].sockfd);
    clients[index].sockfd = -1;
    bzero(clients[index].username, sizeof(clients[index].username));
    clients[index].filenames.clear();
    close(connfd);
    return (NULL);
}
