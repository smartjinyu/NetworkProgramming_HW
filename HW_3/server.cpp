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
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#define MAXLINE 4096
#define LISTENQ 1024
#define MAXCLIENTS 128
#define MAXNAMELEN 256

struct file_info {
    char fileName[MAXNAMELEN] = {0};
    long fileSize = -1;
};

struct clientInfo {
    char username[MAXNAMELEN] = {0};
    int sockfd = -1;
    std::vector<file_info> files;
};

/*
 * return
 * 0 file not exits
 * > 0 file size
 */

long isFileExists(std::vector<file_info> *files, char *filename) {
    for (int i = 0; i < (*files).size(); i++) {
        if (strcmp((*files)[i].fileName, filename) == 0) {
            return (*files)[i].fileSize;
        }
    }
    return 0;
}


clientInfo clients[MAXCLIENTS];
std::vector<file_info> serverFiles;

void setReUseAddr(int sockfd) {
    int flag = 1;
    socklen_t optlen = sizeof(flag);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, optlen);
}

long getFileSize(char *fileName) {
    struct stat st;
    stat(fileName, &st);
    return st.st_size;
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
                file_info file;
                strcpy(file.fileName, ent->d_name);
                file.fileSize = getFileSize(ent->d_name);
                serverFiles.push_back(file);
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

void *servFunc(void *);

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
            // recvline is like "<HW_3.cbp,9296;Client,97384;Server,77504;CMakeCache.txt,33107;>"
            int i; // i means the previous ';'
            int j = 12; // j means the current ';',12 is <
            clients[index].files.clear();
            char sizeBuf[256] = {0};
            for (int k = 12; buff[k] != '>'; k++) {
                if (buff[k] == ';') {
                    file_info mFile;
                    i = j;
                    j = k;
                    int t;
                    for (t = i; buff[t] != ',' && t <= j; t++);// t is the position of ,
                    strncpy(mFile.fileName, buff + i + 1, (size_t) (t - i - 1));
                    bzero(sizeBuf, sizeof(sizeBuf));
                    strncpy(sizeBuf, buff + t + 1, (size_t) (j - t - 1));
                    mFile.fileSize = atol(sizeBuf);
                    clients[index].files.push_back(mFile);
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
                char sizeBuf[256] = {0};
                for (int j = 0; j < serverFiles.size(); j++) {
                    bzero(sendline, sizeof(sendline));
                    bzero(sizeBuf, sizeof(sizeBuf));
                    strcpy(sendline, "filename:");
                    strcat(sendline, serverFiles[j].fileName);
                    strcat(sendline, ",size:");
                    sprintf(sizeBuf, "%ld", serverFiles[j].fileSize);
                    strncat(sendline, sizeBuf, strlen(sizeBuf));
                    strcat(sendline, "\n");
                    write(sockfd, sendline, strlen(sendline));

                }
            } else {
                if (0 <= i < MAXCLIENTS && clients[i].sockfd != -1) {
                    sprintf(sendline, "Listing files in client %s\n", clients[i].username);
                    write(sockfd, sendline, strlen(sendline));
                    char sizeBuf[256] = {0};
                    for (int j = 0; j < clients[i].files.size(); j++) {
                        bzero(sendline, sizeof(sendline));
                        bzero(sizeBuf, sizeof(sizeBuf));
                        strcpy(sendline, "filename:");
                        strcat(sendline, clients[i].files[j].fileName);
                        strcat(sendline, ",size:");
                        sprintf(sizeBuf, "%ld", clients[i].files[j].fileSize);
                        strncat(sendline, sizeBuf, strlen(sizeBuf));
                        strcat(sendline, "\n");
                        write(sockfd, sendline, strlen(sendline));
                    }
                } else {
                    sprintf(sendline, "The client index %d is not valid!\n", i);
                    fputs(sendline, stderr);
                    write(sockfd, sendline, strlen(sendline));

                }
            }
        }

        if (strncmp(recvline, "download:", 9) == 0) {
            // send file to client using p2p
            char name[MAXNAMELEN] = {0};
            char sendline[MAXLINE] = {0};
            strncpy(name, recvline + 9, strlen(recvline) - 10);
            long fileExistsOnServer = isFileExists(&serverFiles, name);
            std::vector<int> clientIndex;
            long fileSize = fileExistsOnServer;
            for (int i = 0; i < MAXCLIENTS; i++) {
                long result = 0;
                if (i != index && clients[i].sockfd != -1 && (result = isFileExists(&clients[i].files, name)) != 0) {
                    clientIndex.push_back(i);
                    fileSize = result;
                }
            }
            if (fileExistsOnServer == 0 && clientIndex.size() == 0) {
                // no such file exists in server and all clients
                sprintf(sendline, "%s not exists in server and all clients\n", name);
                fputs(sendline, stderr);
                write(sockfd, sendline, sizeof(sendline));
            } else if (fileExistsOnServer == 0 && clientIndex.size() != 0) {
                // server does not have this file, all data sent from other clients

                struct sockaddr_in curClientAddr;
                bzero(&curClientAddr, sizeof(curClientAddr));
                socklen_t len = sizeof(curClientAddr);
                getpeername(sockfd, (struct sockaddr *) &curClientAddr, &len);
                // info of the client who will receive file

                long chunkSize = fileSize / clientIndex.size(); // file chunk size each client need to transfer
                printf("chunkSize = %ld, sent from clients\n", chunkSize);
                long offset = 0;
                for (int i = 0; i < clientIndex.size() - 1; i++) {
                    bzero(sendline, sizeof(sendline));
                    sprintf(sendline, "sendto:%s,%d,%s,%ld,%ld\n",
                            inet_ntoa(curClientAddr.sin_addr), ntohs(curClientAddr.sin_port), name, offset, chunkSize);
                    // command is like "sendto:recvip,recvport,filename,offset,size\n"
                    write(clients[clientIndex[i]].sockfd, sendline, strlen(sendline));
                    offset += chunkSize;
                }

                // last client needs additional consideration
                bzero(sendline, sizeof(sendline));
                sprintf(sendline, "sendto:%s,%d,%s,%ld,%ld\n",
                        inet_ntoa(curClientAddr.sin_addr), ntohs(curClientAddr.sin_port), name, offset,
                        fileSize - offset);
                // command is like "sendto:recvip,recvport,filename,offset,size\n"
                write(clients[clientIndex[clientIndex.size() - 1]].sockfd, sendline, strlen(sendline));
            } else {
                // server has this file, data sent from server and other clients
                long serverToTrans = 0;
                if (clientIndex.size() == 0) {
                    // only server has this file
                    serverToTrans = fileSize;
                } else {
                    // server and other clients have this file

                    struct sockaddr_in curClientAddr;
                    bzero(&curClientAddr, sizeof(curClientAddr));
                    socklen_t len = sizeof(curClientAddr);
                    getpeername(sockfd, (struct sockaddr *) &curClientAddr, &len);
                    // info of the client who will receive file

                    long chunkSize =
                            fileSize / (clientIndex.size() + 1); // file chunk size each client need to transfer
                    printf("chunkSize = %ld, sent from clients and server\n", chunkSize);
                    long offset = chunkSize;// [0, chunkSize - 1] will be transferred be server
                    serverToTrans = chunkSize;
                    for (int i = 0; i < clientIndex.size() - 1; i++) {
                        bzero(sendline, sizeof(sendline));
                        sprintf(sendline, "sendto:%s,%d,%s,%ld,%ld\n",
                                inet_ntoa(curClientAddr.sin_addr), ntohs(curClientAddr.sin_port), name, offset,
                                chunkSize);
                        // command is like "sendto:recvip,recvport,filename,offset,size\n"
                        write(clients[clientIndex[i]].sockfd, sendline, strlen(sendline));
                        offset += chunkSize;
                    }

                    // last client needs additional consideration
                    bzero(sendline, sizeof(sendline));
                    sprintf(sendline, "sendto:%s,%d,%s,%ld,%ld\n",
                            inet_ntoa(curClientAddr.sin_addr), ntohs(curClientAddr.sin_port), name, offset,
                            fileSize - offset);
                    // command is like "sendto:recvip,recvport,filename,offset,size\n"
                    write(clients[clientIndex[clientIndex.size() - 1]].sockfd, sendline, strlen(sendline));

                }

                bzero(sendline, sizeof(sendline));
                sprintf(sendline, "sendtoSer:%s,%d,%ld\n", name, 0, serverToTrans);
                // command is like "sendtoSer:filename,offset,size\n"
                write(sockfd, sendline, strlen(sendline));
                if (read(sockfd, sendline, MAXLINE) != 0 && strncmp(sendline, "confirm", 7) == 0) {
                    // ack from receiver, begin to transfer
                    int f = open(name, O_RDONLY);
                    off_t offset = 0;
                    ssize_t len = 0;
                    long sent_data = 0;
                    while ((len = sendfile(sockfd, f, &offset, MAXLINE)) > 0) {
                        sent_data += len;
                        //printf("sent len = %d, total = %d\n",len,sent_data);
                        if (sent_data >= serverToTrans) {
                            break;
                        }
                    }
                    close(f);
                } else {
                    bzero(sendline, sizeof(sendline));
                    strcpy(sendline, "Not receive confirm message from p2p receiver\n");
                    fputs(sendline, stderr);
                }


            }
        }

        bzero(recvline, sizeof(recvline));
    }

    if (n < 0 && errno == EINTR) {
        goto again; // ignore EINTR
    } else if (n < 0) {
        fprintf(stderr, "servRecv:read error");
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
        pthread_create(&tid, NULL, &servFunc, arg); /* pass by value*/
    }
}

#pragma clang diagnostic pop

void *servFunc(void *arg) {
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
    clients[index].files.clear();
    close(connfd);
    return (NULL);
}
