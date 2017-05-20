//
// Created by smartjinyu on 5/19/17.
//

#include <netinet/in.h>
#include <memory.h>
#include <cstdio>
#include <cstdlib>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#define MAXLINE 4096
#define MAXNAMELEN 256
#define LISTENQ 1024
#define MAXCLIENTS 128


struct clientInfo {
    char username[MAXNAMELEN] = {0};
    int sockfd = -1;
    std::vector<std::string> filenames;
};

clientInfo clients[MAXCLIENTS];

struct sendFileParam {
    char ip[MAXNAMELEN] = {0};
    int port;
    char filename[MAXNAMELEN] = {0};
    long offset;
    long filesize;
};

int sockfdClient; /* global for both threads to access */
FILE *fpStdin;


char clientName[MAXNAMELEN] = {0};

void *clientInput(void *);

void *servListen(void *);

long getFileSize(char *fileName) {
    struct stat st;
    stat(fileName, &st);
    return st.st_size;
}

void listfiles(int sockfd) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(".")) != NULL) {
        /* print all the files and directories within directory */
        char sendline[MAXLINE] = {0};
        char buf[256] = {0};// file size to char *
        strcpy(sendline, "listMyfiles:<");
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                // regular file
                bzero(buf, sizeof(buf));
                sprintf(buf, "%ld", getFileSize(ent->d_name));
                strcat(sendline, ent->d_name);
                strcat(sendline, ",");
                strncat(sendline, buf, strlen(buf));
                strcat(sendline, ";");
            }
        }
        sendline[strlen(sendline)] = '>';
        // sendline is like "<HW_3.cbp,9296;Client,97384;Server,77504;CMakeCache.txt,33107;>"
        write(sockfd, sendline, strlen(sendline));
        closedir(dir);
    } else {
        /* could not open directory */
        perror("");
        return;
    }

}

void *connectAndSendFile(void *arg) {
    sendFileParam param = *((sendFileParam *) arg);
    //printf("param ip = %s, port = %d\n",param.ip,param.port);
    int p2pClientSocket = socket(AF_INET, SOCK_STREAM, 0);
    int socketOpOn = 1;
    struct sockaddr_in clientAddr;
    bzero(&clientAddr, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons((uint16_t) param.port);
    inet_pton(AF_INET, param.ip, &clientAddr.sin_addr);
    setsockopt(p2pClientSocket, SOL_SOCKET, SO_REUSEADDR, &socketOpOn, sizeof(socketOpOn));
    if (connect(p2pClientSocket, (struct sockaddr *) &clientAddr, sizeof(clientAddr)) < 0) {
        fprintf(stderr, "Connect to p2p server failed, error message = %s\n", strerror(errno));
    }
    char sendline[MAXLINE] = {0};
    sprintf(sendline, "p2psend:%s,%ld,%ld\n", param.filename, param.offset, param.filesize);
    write(p2pClientSocket, sendline, strlen(sendline));
    // send file info to receiver

    bzero(sendline, sizeof(sendline));
    if (read(p2pClientSocket, sendline, MAXLINE) != 0 && strncmp(sendline, "confirm", 7) == 0) {
        // ack from receiver, begin to transfer
        int f = open(param.filename, O_RDONLY);
        off_t offset = param.offset;
        ssize_t len = 0;
        long sent_data = 0;
        while ((len = sendfile(p2pClientSocket, f, &offset, MAXLINE)) > 0) {
            sent_data += len;
            //printf("sent len = %d, total = %d\n",len,sent_data);
            if (sent_data >= param.filesize) {
                break;
            }
        }
        close(f);
    } else {
        bzero(sendline, sizeof(sendline));
        strcpy(sendline, "Not receive confirm message from p2p receiver\n");
        fputs(sendline, stderr);
    }
    close(p2pClientSocket);
    return (NULL);
}

void cliFunc(FILE *fp_arg) {
    char recvline[MAXLINE] = {0};
    pthread_t tid;

    fpStdin = fp_arg;

    pthread_create(&tid, NULL, clientInput, NULL);
    while (read(sockfdClient, recvline, MAXLINE) > 0) {
        // receive from server
        fputs(recvline, stdout);
        fflush(stdout);

        if (strncmp(recvline, "sendto:", 7) == 0) {
            // connect to other client and send file to it
            // command is like "sendto:recvip,recvport,filename,offset,size\n"
            printf("\n");
            sendFileParam param;
            char clientPortc[MAXNAMELEN] = {0};
            char offsetc[MAXNAMELEN] = {0};
            char sizec[MAXNAMELEN] = {0};
            int i = 6, j = 6; // i is previous ',' , j is current ','
            for (; recvline[j] != ','; j++);
            strncpy(param.ip, recvline + i + 1, (size_t) (j - i - 1));
            i = j++;

            for (; recvline[j] != ','; j++);
            strncpy(clientPortc, recvline + i + 1, (size_t) (j - i - 1));
            i = j++;
            param.port = atoi(clientPortc);

            for (; recvline[j] != ','; j++);
            strncpy(param.filename, recvline + i + 1, (size_t) (j - i - 1));
            i = j++;

            for (; recvline[j] != ','; j++);
            strncpy(offsetc, recvline + i + 1, (size_t) (j - i - 1));
            i = j++;
            param.offset = atol(offsetc);

            for (; recvline[j] != '\n'; j++);
            strncpy(sizec, recvline + i + 1, (size_t) (j - i - 1));
            param.filesize = atol(sizec);
            // parse command
            pthread_t tid1;
            pthread_create(&tid1, NULL, connectAndSendFile, &param);

        }
        if (strncmp(recvline, "sendtoSer:", 10) == 0) {
            // receive file from server
            // sendtoSer:filename,offset,filesize
            char filename[MAXNAMELEN] = {0};
            char offsetc[MAXNAMELEN] = {0};
            char sizec[MAXNAMELEN] = {0};
            int i = 9, j = 9; // i is previous ',' , j is current ','
            for (; recvline[j] != ','; j++);
            strncpy(filename, recvline + i + 1, (size_t) (j - i - 1));
            i = j++;

            for (; recvline[j] != ','; j++);
            strncpy(offsetc, recvline + i + 1, (size_t) (j - i - 1));
            i = j++;

            for (; recvline[j] != '\n'; j++);
            strncpy(sizec, recvline + i + 1, (size_t) (j - i - 1));
            long filesize = atol(sizec);
            // parse data from command

            write(sockfdClient, "confirm\n", 8);// send ack to sender
            FILE *recvFile = fopen(filename, "r+");
            if (recvFile == NULL) {
                //printf("file not exists\n");
                recvFile = fopen(filename, "w+");
                // w+ mode will discard file content already exists
            }

            if (recvFile != NULL) {
                if (fseek(recvFile, atol(offsetc), SEEK_SET) < 0) {
                    fprintf(stderr, "fseek error, error = %s\n", strerror(errno));
                } // move to offset

                char recvbuff[MAXLINE] = {0};
                ssize_t len = 0;
                long data_received = 0;
                while ((len = recv(sockfdClient, recvbuff, MAXLINE, 0)) > 0) {
                    data_received += len;
                    //printf("recv = %s\n",recvbuff);
                    if (data_received < filesize) {
                        fwrite(recvbuff, sizeof(char), (size_t) len, recvFile);
                        //printf("recv0 len = %d, total = %d\n",len,data_received);
                    } else {
                        fwrite(recvbuff, sizeof(char), (size_t) (filesize - data_received + len), recvFile);
                        //printf("recv1 len = %d, total = %d\n",filesize - data_received + len,data_received);

                        // only write required size of data
                        break;
                    }
                    bzero(recvbuff, sizeof(recvbuff));
                }
                fflush(recvFile);
                fclose(recvFile);

            } else {
                fputs("error when opening file", stderr);
            }

        }

        bzero(recvline, sizeof(recvline));
    }
}

void showHelpMenu() {
    printf("------------ Help Menu -------------\n");
    printf("help: show help menu\n");
    printf("listclients: list all the clients online\n");
    printf("listfiles:<index> list files of clients of index (-1 on the server)\n");
    printf("download:<filename> download file from server and other clients\n");
    printf("------------ Help Menu -------------\n");
}

int main(int argc, char **argv) {
    struct sockaddr_in servaddr;
    const int socketOpOn = 1;
    if (argc != 3) {
        fprintf(stderr, "Wrong arguments! Please run ./Client <ip> <port>\n");
        exit(-1);
    }

    // client socket
    bzero(&servaddr, sizeof(servaddr));
    sockfdClient = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sockfdClient, SOL_SOCKET, SO_REUSEADDR, &socketOpOn, sizeof(socketOpOn));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons((uint16_t) atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        // translate the ip address
        fprintf(stderr, "inet_pton error for %s", argv[1]);
        return -1;
    }
    if (connect(sockfdClient, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "Connect failed, error message = %s\n", strerror(errno));
    }
    showHelpMenu();
    printf("Please input the client name: ");
    fflush(stdout);


    pthread_t tid;
    pthread_create(&tid, NULL, servListen, NULL);

    listfiles(sockfdClient);
    cliFunc(stdin);
    close(sockfdClient);
    return 0;
}

void *clientInput(void *arg) {
    char sendline[MAXLINE] = {0};

    while (fgets(sendline, MAXLINE, fpStdin) != NULL) {
        if (clientName[0] == 0) {
            // name unset
            strncpy(clientName, sendline, strlen(sendline) - 1); // last character is \n
            bzero(sendline, sizeof(sendline));
            strcpy(sendline, "name:");
            strcat(sendline, clientName);
            strcat(sendline, "\n");
        }
        if (strncmp(sendline, "help", 4) == 0) {
            showHelpMenu();
            continue;
        }
        write(sockfdClient, sendline, strlen(sendline));
        bzero(sendline, sizeof(sendline));

    }
    return (NULL);
}


void showClientTerminatedInfo(int sockfd) {
    struct sockaddr_in terminatedAddr;
    bzero(&terminatedAddr, sizeof(terminatedAddr));
    socklen_t len = sizeof(terminatedAddr);
    getpeername(sockfd, (struct sockaddr *) &terminatedAddr, &len);
    printf("Client terminated, ip = %s, port = %d\n",
           inet_ntoa(terminatedAddr.sin_addr), ntohs(terminatedAddr.sin_port));
}


void servRecv(int index) {
    ssize_t n;
    char recvline[MAXLINE] = {0};
    int sockfd = clients[index].sockfd;
    again:
    while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
        fputs(recvline, stdout);
        if (strncmp(recvline, "p2psend:", 8) == 0) {
            // receive file from client
            // p2psend:filename,offset,filesize
            char filename[MAXNAMELEN] = {0};
            char offsetc[MAXNAMELEN] = {0};
            char sizec[MAXNAMELEN] = {0};
            int i = 7, j = 7; // i is previous ',' , j is current ','
            for (; recvline[j] != ','; j++);
            strncpy(filename, recvline + i + 1, (size_t) (j - i - 1));
            i = j++;

            for (; recvline[j] != ','; j++);
            strncpy(offsetc, recvline + i + 1, (size_t) (j - i - 1));
            i = j++;

            for (; recvline[j] != '\n'; j++);
            strncpy(sizec, recvline + i + 1, (size_t) (j - i - 1));
            long filesize = atol(sizec);
            // parse data from command

            write(sockfd, "confirm\n", 8);// send ack to sender
            FILE *recvFile = fopen(filename, "r+");
            if (recvFile == NULL) {
                //printf("file not exists\n");
                recvFile = fopen(filename, "w+");
                // w+ mode will discard file content already exists
            }

            if (recvFile != NULL) {
                if (fseek(recvFile, atol(offsetc), SEEK_SET) < 0) {
                    fprintf(stderr, "fseek error, error = %s\n", strerror(errno));
                } // move to offset

                char recvbuff[MAXLINE] = {0};
                ssize_t len = 0;
                long data_received = 0;
                while ((len = recv(sockfd, recvbuff, MAXLINE, 0)) > 0) {
                    data_received += len;
                    //printf("recv = %s\n",recvbuff);
                    if (data_received < filesize) {
                        fwrite(recvbuff, sizeof(char), (size_t) len, recvFile);
                        //printf("recv0 len = %d, total = %d\n",len,data_received);
                    } else {
                        fwrite(recvbuff, sizeof(char), (size_t) (filesize - data_received + len), recvFile);
                        //printf("recv1 len = %d, total = %d\n",filesize - data_received + len,data_received);

                        // only write required size of data
                        break;
                    }
                    bzero(recvbuff, sizeof(recvbuff));
                }
                fflush(recvFile);
                fclose(recvFile);

            } else {
                fputs("error when opening file", stderr);
            }

        }

        //write(sockfd, recvline, strlen(recvline));
        bzero(recvline, sizeof(recvline));
    }

    if (n < 0 && errno == EINTR) {
        goto again; // ignore EINTR
    } else if (n < 0) {
        fprintf(stderr, "servRecv:read error");
        return;
    }
}


void *servFunc(void *arg) {
    int index;
    index = *((int *) arg);
    free(arg);
    pthread_detach(pthread_self());
    int connfd = clients[index].sockfd;
    servRecv(index);

    // thread terminated
    showClientTerminatedInfo(clients[index].sockfd);
    clients[index].sockfd = -1;
    bzero(clients[index].username, sizeof(clients[index].username));
    clients[index].filenames.clear();
    close(connfd);
    return (NULL);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

void *servListen(void *arg) {
    int CSlistenfd = -1, CSconnectfd = -1, CSclientfd = -1;
    struct sockaddr_in serverAddr, CSclientAddr;
    const int socketOpOn = 1;
    socklen_t clientAddrLen;
    pthread_t tid;


    CSlistenfd = socket(AF_INET, SOCK_STREAM, 0);
    // printf("CSlistenfd = %d\n",CSlistenfd);
    bzero(&serverAddr, sizeof(serverAddr));
    socklen_t len = sizeof(serverAddr);
    getsockname(sockfdClient, (struct sockaddr *) &serverAddr, &len);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    setsockopt(CSlistenfd, SOL_SOCKET, SO_REUSEADDR, &socketOpOn, sizeof(socketOpOn));
    if (bind(CSlistenfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        fprintf(stderr, "Bind error, error = %s", strerror(errno));
    }

    if (listen(CSlistenfd, LISTENQ) < 0) {
        printf("Error when listen, error = %s\n", strerror(errno));
        fprintf(stderr, "Listen error, error = %s", strerror(errno));

    }
    for (;;) {
        bzero(&CSclientAddr, sizeof(CSclientAddr));
        clientAddrLen = sizeof(CSclientAddr);
        CSconnectfd = accept(CSlistenfd, (struct sockaddr *) &CSclientAddr, &clientAddrLen);
        int i = 0;
        for (i = 0; i < MAXCLIENTS; i++) {
            if (clients[i].sockfd == -1) {
                clients[i].sockfd = CSconnectfd;
                break;
            }
        }
        printf("New client connected, index = %d, ip = %s, port = %d\n",
               i, inet_ntoa(CSclientAddr.sin_addr), ntohs(CSclientAddr.sin_port));

        int *argcc = (int *) malloc(sizeof(*argcc));
        *argcc = i;
        pthread_create(&tid, NULL, &servFunc, argcc); /* pass by value*/
    }

}

#pragma clang diagnostic pop