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
#include <string>
#include <map>

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

std::map<std::string, int> p2pConnections;
// threads receiving files now, key: filename, value: connections, 0 represents file transfer succeed

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

void sendFileToClient(int index, char *filename) {
    // send file to client using p2p, index is the client to receive file
    char sendline[MAXLINE] = {0};
    int sockfd = clients[index].sockfd;
    long fileExistsOnServer = isFileExists(&serverFiles, filename);
    std::vector<int> clientIndex;
    long fileSize = fileExistsOnServer;
    for (int i = 0; i < MAXCLIENTS; i++) {
        long result = 0;
        if (i != index && clients[i].sockfd != -1 && (result = isFileExists(&clients[i].files, filename)) != 0) {
            clientIndex.push_back(i);
            fileSize = result;
        }
    }
    if (fileExistsOnServer == 0 && clientIndex.size() == 0) {
        // no such file exists in server and all clients
        sprintf(sendline, "%s not exists in server or any clients\n", filename);
        fputs(sendline, stderr);
        write(sockfd, sendline, strlen(sendline));
    } else if (fileExistsOnServer == 0 && clientIndex.size() != 0) {
        // server does not have this file, all data sent from other clients

        struct sockaddr_in curClientAddr;
        bzero(&curClientAddr, sizeof(curClientAddr));
        socklen_t len = sizeof(curClientAddr);
        getpeername(sockfd, (struct sockaddr *) &curClientAddr, &len);
        // info of the client who will receive file

        long chunkSize = fileSize / clientIndex.size(); // file chunk size each client need to transfer
        printf("chunkSize = %ld, sent from clients in %ld chunks\n", chunkSize, clientIndex.size());
        long offset = 0;
        for (int i = 0; i < clientIndex.size() - 1; i++) {
            bzero(sendline, sizeof(sendline));
            sprintf(sendline, "sendto:%s,%d,%s,%ld,%ld\n",
                    inet_ntoa(curClientAddr.sin_addr), ntohs(curClientAddr.sin_port), filename, offset, chunkSize);
            // command is like "sendto:recvip,recvport,filename,offset,size\n"
            write(clients[clientIndex[i]].sockfd, sendline, strlen(sendline));
            offset += chunkSize;
        }

        // last client needs additional consideration
        bzero(sendline, sizeof(sendline));
        sprintf(sendline, "sendto:%s,%d,%s,%ld,%ld\n",
                inet_ntoa(curClientAddr.sin_addr), ntohs(curClientAddr.sin_port), filename, offset,
                fileSize - offset);
        // command is like "sendto:recvip,recvport,filename,offset,size\n"
        write(clients[clientIndex[clientIndex.size() - 1]].sockfd, sendline, strlen(sendline));
    } else {
        // server has this file, data sent from server and other clients
        long serverToTrans = 0;
        if (clientIndex.size() == 0) {
            // only server has this file
            serverToTrans = fileSize;
            printf("sent from  server in 1 chunk\n");

        } else {
            // server and other clients have this file

            struct sockaddr_in curClientAddr;
            bzero(&curClientAddr, sizeof(curClientAddr));
            socklen_t len = sizeof(curClientAddr);
            getpeername(sockfd, (struct sockaddr *) &curClientAddr, &len);
            // info of the client who will receive file

            long chunkSize =
                    fileSize / (clientIndex.size() + 1); // file chunk size each client need to transfer
            printf("chunkSize = %ld, sent from clients and server in %ld chunks\n", chunkSize, clientIndex.size() + 1);
            long offset = chunkSize;// [0, chunkSize - 1] will be transferred be server
            serverToTrans = chunkSize;
            for (int i = 0; i < clientIndex.size() - 1; i++) {
                bzero(sendline, sizeof(sendline));
                sprintf(sendline, "sendto:%s,%d,%s,%ld,%ld\n",
                        inet_ntoa(curClientAddr.sin_addr), ntohs(curClientAddr.sin_port), filename, offset,
                        chunkSize);
                // command is like "sendto:recvip,recvport,filename,offset,size\n"
                write(clients[clientIndex[i]].sockfd, sendline, strlen(sendline));
                offset += chunkSize;
            }

            // last client needs additional consideration
            bzero(sendline, sizeof(sendline));
            sprintf(sendline, "sendto:%s,%d,%s,%ld,%ld\n",
                    inet_ntoa(curClientAddr.sin_addr), ntohs(curClientAddr.sin_port), filename, offset,
                    fileSize - offset);
            // command is like "sendto:recvip,recvport,filename,offset,size\n"
            write(clients[clientIndex[clientIndex.size() - 1]].sockfd, sendline, strlen(sendline));

        }

        bzero(sendline, sizeof(sendline));
        sprintf(sendline, "serSendto:%s,%d,%ld\n", filename, 0, serverToTrans);
        // command is like "sendtoSer:filename,offset,size\n"
        write(sockfd, sendline, strlen(sendline));

        // server part will do when client send back ack


    }
}

void sendFileToServer(int index, char *filename) {
    // clients upload file to server, the request is from client of index
    int sockfd = clients[index].sockfd;
    char sendline[MAXLINE] = {0};
    long fileSize = 0;
    std::vector<int> clientIndex;
    for (int i = 0; i < MAXCLIENTS; i++) {
        long result = 0;
        if (clients[i].sockfd != -1 && (result = isFileExists(&clients[i].files, filename)) != 0) {
            clientIndex.push_back(i);
            fileSize = result;
        }
    }
    if (clientIndex.size() == 0) {
        // no such file in any client
        sprintf(sendline, "%s not exists in any clients\n", filename);
        fputs(sendline, stderr);
        write(sockfd, sendline, strlen(sendline));
    } else {
        // ask clients to transfer the file
        long chunkSize = fileSize / clientIndex.size(); // file chunk size each client need to transfer
        printf("chunkSize = %ld, sent from clients in %ld chunks To server\n", chunkSize, clientIndex.size());
        long offset = 0;
        for (int i = 0; i < clientIndex.size() - 1; i++) {
            bzero(sendline, sizeof(sendline));
            sprintf(sendline, "uploadSer:%s,%ld,%ld\n", filename, offset, chunkSize);
            // command is like "uploadSer:filename,offset,size\n"
            write(clients[clientIndex[i]].sockfd, sendline, strlen(sendline));
            offset += chunkSize;
        }

        // last client needs additional consideration
        bzero(sendline, sizeof(sendline));
        sprintf(sendline, "uploadSer:%s,%ld,%ld\n", filename, offset, fileSize - offset);
        // command is like "uploadSer:filename,offset,size\n"
        write(clients[clientIndex[clientIndex.size() - 1]].sockfd, sendline, strlen(sendline));

    }

}


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
                    sprintf(sendline, "Invalid client index %d\n", i);
                    fputs(sendline, stderr);
                    write(sockfd, sendline, strlen(sendline));

                }
            }
        }

        if (strncmp(recvline, "download:", 9) == 0) {
            // send file to client using p2p
            // download:filename
            char name[MAXNAMELEN] = {0};
            strncpy(name, recvline + 9, strlen(recvline) - 10);
            sendFileToClient(index, name);
        }

        if (strncmp(recvline, "upload:", 7) == 0) {
            // send file to client using p2p
            // upload:filename,index
            char name[MAXNAMELEN] = {0};
            int i = 0;
            for (; recvline[i] != ','; i++);
            strncpy(name, recvline + 7, (size_t) (i - 7));
            char mIndexc[MAXNAMELEN] = {0};
            strncpy(mIndexc, recvline + i + 1, (size_t) (strlen(recvline) - i - 2));
            int mIndex = atoi(mIndexc);
            if (mIndex == -1) {
                // send file to server
                sendFileToServer(index, name);
                // the index here is the upload request origin
            } else if (0 <= mIndex < MAXCLIENTS && clients[mIndex].sockfd != -1) {
                // normal transfer to a client
                sendFileToClient(mIndex, name);
            } else {
                // invalid client index
                char sendline[MAXLINE] = {0};
                sprintf(sendline, "Invalid client index %d\n", mIndex);
                fputs(sendline, stderr);
                write(sockfd, sendline, strlen(sendline));

            }

        }

        if (strncmp(recvline, "uploadSer:", 10) == 0) {
            // receive file from clients
            // command is like "uploadSer:filename,offset,size\n"
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
            // parse command
            char sendline[MAXLINE] = {0};
            sprintf(sendline, "upToSer:%s,%ld,%ld\n", filename, atol(offsetc), filesize);
            write(sockfd, sendline, strlen(sendline));
            // send ack to client
            std::map<std::string, int>::iterator iterator = p2pConnections.find(filename);
            if (iterator == p2pConnections.end()) {
                p2pConnections.insert(std::pair<std::string, int>(filename, 1));
            } else {
                iterator->second++;
            }
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

                iterator = p2pConnections.find(filename);
                iterator->second--;
                if (iterator->second == 0) {
                    listfiles();
                    p2pConnections.erase(iterator);
                    printf("upload file to server succeeded!\n");
                }

            } else {
                fputs("error when opening file", stderr);
            }


        }


        if (strncmp(recvline, "serSendto:", 10) == 0) {
            // ack from client, send part of file to it
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
            int f = open(filename, O_RDONLY);
            off_t offset = atol(offsetc);
            ssize_t len = 0;
            long sent_data = 0;
            while ((len = sendfile(sockfd, f, &offset, MAXLINE)) > 0) {
                sent_data += len;
                //printf("sent len = %d, total = %d\n",len,sent_data);
                if (sent_data >= filesize) {
                    break;
                }
            }
            close(f);
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
