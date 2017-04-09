//
// Created by smartjinyu on 3/31/17.
//

#include <sys/param.h>
#include <netinet/in.h>
#include <memory.h>
#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#define MAXLINE 2048
#define LISTENQ 1024
char curDir[FD_SETSIZE][128];
char defaultDir[128];

int listFiles(int sockfd) {
    char buf[MAXLINE] = {0};
    FILE *fp;

    //bzero(&buf,sizeof(buf));

    if (getcwd(buf, sizeof(buf)) != NULL) {
        write(sockfd, "List files in ", 14);
        write(sockfd, buf, strlen(buf));
        write(sockfd, "\n", 2);
    } else {
        write(sockfd, "Something went wrong...\n", 25);
        return -1;
    }
    if ((fp = popen("ls -a", "r")) == NULL) {
        write(sockfd, "Something went wrong...\n", 25);
        return -1;
    }

    bzero(&buf, sizeof(buf));
    while (fgets(buf, MAXLINE, fp) != NULL) {
        write(sockfd, buf, strlen(buf));
    }

    if (pclose(fp)) {
        return -1;
    }
    return 0;
}

int changeDir(char path[], int sockfd, int i) {
    if (chdir(path) == 0) {
        bzero(&curDir[i], sizeof(curDir[i]));
        if (getcwd(curDir[i], sizeof(curDir[i])) != NULL) {
            write(sockfd, "Current path is ", 16);
            write(sockfd, curDir[i], strlen(curDir[i]));
            write(sockfd, "\n", 2);
        } else {
            write(sockfd, "Something went wrong...\n", 25);
            return -1;
        }
        return 0;
    } else {
        write(sockfd, "Something went wrong...\n", 25);
        return -1;
    }
}

int downloadFile(char *filename, int sockfd, int i) {
    /*
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        write(sockfd, "Failed to open file:", 20);
        write(sockfd, strerror(errno), strlen(strerror(errno))+1);
        fprintf(stderr, "Failed to open file %s: %s\n", filename, strerror(errno));
        return -1;
    }
    struct stat stat_buf;// file size to be sent
    fstat(fd, &stat_buf);
    off_t offset = 0;


    char header[256] = {0};
    sprintf(header, "get:%s,%d", filename, (int) stat_buf.st_size);
    write(sockfd, header, strlen(header)+1);

    int remain_data = (int) stat_buf.st_size;
    //printf("filesize = %d\n",remain_data);
    ssize_t len;
    while ((len = sendfile(sockfd, fd, &offset, stat_buf.st_size)) > 0) {
        remain_data -= len;
        //printf("len = %d, remaining = %d\n",len,remain_data);

    }
    //printf("last len = %d\n",len);
    close(fd);
    //printf("send successfully!\n");

    */
    int f = open(filename, O_RDONLY);
    if (f < 0) {
        char header[256] = {0};
        sprintf(header, "get:%s,%d", filename, -1);
        write(sockfd, header, strlen(header) + 1);
        write(sockfd, "Failed to open file:", 20);
        write(sockfd, strerror(errno), strlen(strerror(errno)) + 1);
        fprintf(stderr, "Failed to open file %s: %s\n", filename, strerror(errno));
        return -1;
    }
    struct stat st;
    stat(filename, &st);
    char header[256] = {0};
    sprintf(header, "get:%s,%d", filename, (int) st.st_size);
    //printf("filesize = %d\n",(int)st.st_size);
    write(sockfd, header, strlen(header) + 1);
    int sent = 0, remain = (int) st.st_size;
    //printf("sending files... \n");
    ssize_t read_bytes, sent_bytes;
    char sendbuf[MAXLINE] = {0};
    while ((read_bytes = read(f, sendbuf, MAXLINE)) > 0) {
        if ((sent_bytes = send(sockfd, sendbuf, (size_t) read_bytes, 0)) < read_bytes) {
            perror("send error");
            return -1;
        }
        sent += sent_bytes;
        remain -= sent_bytes;
        //printf("send = %d, remaining = %d \n",sent_bytes ,remain);
    }
    close(f);
    printf("Sent successfully\n");
    return 0;
}


int uploadFile(char *recvline, int sockfd, int i) {
    // upload file to server
    // recvline: put:test.c,255

    char filename[256] = {0}, filesize[256] = {0};
    int file_size = 0;
    int j;
    for (j = 4; recvline[j] != ','; j++) {
        filename[j - 4] = recvline[j];
    }
    j++;
    int k;
    for (k = 0; recvline[j] != 0; j++, k++) {
        filesize[k] = recvline[j];
    }
    file_size = atoi(filesize);
    //printf("recvline = %s\n",recvline);
    //printf("filename = %s, filesize = %d\n",filename,file_size);
    if (file_size != -1) {
        // file_size == 0 means something is wrong
        if (remove(filename) != 0) {
            //perror( "Error deleting file\n" );
        } else {
            //puts( "File successfully deleted\n" );
        }
        FILE *recvFile;
        recvFile = fopen(filename, "w+");
        if (recvFile != NULL) {
            ssize_t len;
            bzero(recvline, sizeof(recvline));
            int received_data = 0;
            while (1) {
                //printf("recv started!\n");
                len = recv(sockfd, recvline, MAXLINE, 0);
                if (len <= 0) {
                    break;
                }
                //printf("recv completed!\n");
                received_data += len;
                //printf("len = %d, received = %d\n", (int) len, received_data);
                fwrite(recvline, sizeof(char), (size_t) len, recvFile);
                fflush(recvFile);

                //printf("fwrite completed!\n");
                if (received_data >= file_size) {
                    break;
                }
                bzero(recvline, sizeof(recvline));

            }
            printf("Upload Complete!\n");
            write(sockfd, "Upload Complete!\n", 17);
            fclose(recvFile);
            return 0;
        } else {
            fputs("error when opening file", stderr);
        }
    } else {
        printf("Failed to upload file to server!");
    }
    write(sockfd, "Upload failed!\n", 15);
    return -1;


}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main(int argc, char **argv) {
    int listenfd, connfd, sockfd, i, maxi, maxfd;
    int nready, client[FD_SETSIZE];
    fd_set rset, allset;
    ssize_t n;
    char recvline[MAXLINE];
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen;

    if (argc != 2) {
        printf("wrong arguments! Please run ./server.out <port> \n");
        return -1;
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);// for any interface on the server
    serverAddr.sin_port = htons((uint16_t) atoi(argv[1]));

    bind(listenfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    // monitor the request on the port
    listen(listenfd, LISTENQ);
    // LISTENQ the max length of the backlog (= complete + incomplete request queue)
    for (int j = 0; j < FD_SETSIZE; j++) {
        bzero(curDir[j], sizeof(curDir[j]));
    }
    bzero(defaultDir, sizeof(defaultDir));
    getcwd(defaultDir, sizeof(defaultDir));

    maxfd = listenfd;
    maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++) {
        client[i] = -1;
    }
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    while (true) {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        // number of ready descriptors
        if (FD_ISSET(listenfd, &rset)) {
            clientLen = sizeof(clientAddr);
            connfd = accept(listenfd, (struct sockaddr *) &clientAddr, &clientLen);
            printf("New client connected, ip = %s, port = %d\n",
                   inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            // new client connection
            for (i = 0; i < FD_SETSIZE; i++) {
                if (client[i] < 0) {
                    // find the first available descriptor
                    client[i] = connfd;
                    strcpy(curDir[i], defaultDir);
                    break;
                }
            }
            if (i == FD_SETSIZE) {
                printf("too many clients\n");
            }
            FD_SET(connfd, &allset);
            if (connfd > maxfd) {
                maxfd = connfd;
            }
            if (i > maxi) {
                maxi = i;
            }
            if (--nready <= 0) {
                continue;// no more readable descriptors
            }

        }

        for (i = 0; i <= maxi; i++) {
            // check all clients for data
            if ((sockfd = client[i]) < 0) {
                continue; // skip empty client
            }
            if (FD_ISSET(sockfd, &rset)) {
                bzero(&recvline, sizeof(recvline));
                if ((n = read(sockfd, recvline, MAXLINE)) == 0) {
                    // connection closed by client
                    struct sockaddr_in terminatedAddr;
                    socklen_t len = sizeof(terminatedAddr);
                    getpeername(sockfd, (struct sockaddr *) &terminatedAddr, &len);
                    printf("Client terminated, ip = %s, port = %d\n",
                           inet_ntoa(terminatedAddr.sin_addr), ntohs(terminatedAddr.sin_port));

                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                    bzero(curDir[i], sizeof(curDir[i]));

                } else {
                    fputs(recvline, stdout);
                    fflush(stdout);

                    //write(sockfd, recvline, (size_t) n);
                    if (recvline[0] == 'l' && recvline[1] == 's') {
                        // list files
                        chdir(curDir[i]);
                        listFiles(sockfd);
                    } else if (recvline[0] == 'c' && recvline[1] == 'd') {
                        // change directory;
                        chdir(curDir[i]);
                        char dir[128] = {0};
                        bzero(&dir, sizeof(dir));
                        int j;
                        for (j = 3; recvline[j] != '\n' && recvline[j] != '\000'; j++) {
                            dir[j - 3] = recvline[j];
                        }
                        changeDir(dir, sockfd, i);
                    } else if (recvline[0] == 'g' && recvline[1] == 'e' && recvline[2] == 't') {
                        // download file from server
                        chdir(curDir[i]);
                        char filename[128] = {0};
                        for (int j = 4; recvline[j] != '\n' && recvline[j] != '\000'; j++) {
                            filename[j - 4] = recvline[j];
                        }
                        downloadFile(filename, sockfd, i);
                    } else if (recvline[0] == 'p' && recvline[1] == 'u' && recvline[2] == 't') {
                        // upload file to server
                        chdir(curDir[i]);
                        uploadFile(recvline, sockfd, i);
                    } else if (recvline[0] == 'm' && recvline[1] == 'k' && recvline[2] == 'd' && recvline[3] == 'i' &&
                               recvline[4] == 'r') {
                        chdir(curDir[i]);
                        char pathname[128] = {0};
                        for (int j = 6; recvline[j] != '\n' && recvline[j] != '\000'; j++) {
                            pathname[j - 6] = recvline[j];
                        }
                        if (mkdir(pathname, 0777) == 0) {
                            write(sockfd, "Create directory successfully!\n", 31);
                        } else {
                            write(sockfd, "Failed to create dir:", 21);
                            write(sockfd, strerror(errno), strlen(strerror(errno)) + 1);
                            fprintf(stderr, "Failed to create dir %s: %s\n", pathname, strerror(errno));
                        }

                    }
                }
                if (--nready <= 0) {
                    break; // no more readable descriptors
                }

            }
        }
    }
}

#pragma clang diagnostic pop