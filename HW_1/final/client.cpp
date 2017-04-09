
//
// Created by smartjinyu on 3/31/17.
//

#include <netinet/in.h>
#include <memory.h>
#include <arpa/inet.h>
#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/sendfile.h>

#define MAXLINE 2048

int max(int a, int b) {
    return a > b ? a : b;
}


int uploadFile(char *filename, int sockfd) {
    int f = open(filename, O_RDONLY);
    if (f < 0) {
        char header[256] = {0};
        sprintf(header, "put:%s,%d", filename, -1);
        write(sockfd, header, strlen(header) + 1);
        write(sockfd, "Failed to open file:", 20);
        write(sockfd, strerror(errno), strlen(strerror(errno)) + 1);
        fprintf(stderr, "Failed to open file %s: %s\n", filename, strerror(errno));
        return -1;
    }
    struct stat st;
    stat(filename, &st);
    char header[256] = {0};
    sprintf(header, "put:%s,%d", filename, (int) st.st_size);
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
    //printf("Sent successfully\n");
    return 0;
}

void downloadFile(char *recvline, int sockfd) {
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
        FILE *recvFile = NULL;
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
            printf("Download Complete!\n");
            fclose(recvFile);
        } else {
            fputs("error when opening file", stderr);
        }
    } else {
        printf("Failed to download file from server!");
    }

}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

void str_cli(FILE *fp, int sockfd) {
    int maxfdp1, stdineof;
    fd_set rset;
    char sendline[MAXLINE], recvline[MAXLINE];
    bzero(sendline, sizeof(sendline));
    bzero(recvline, sizeof(recvline));
    FD_ZERO(&rset);
    stdineof = 0; // use for test readable, 0:connect
    while (true) {
        if (stdineof == 0) {
            FD_SET(fileno(fp), &rset);// input
        }
        FD_SET(sockfd, &rset);// socket

        maxfdp1 = max(fileno(fp), sockfd) + 1;
        select(maxfdp1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &rset)) {
            // socket is readable
            if (recv(sockfd, recvline, MAXLINE, 0) == 0) {
                printf("recv = 0\n");
                if (stdineof == 1) {
                    return; // normal termination
                } else {
                    printf("Client: Server terminated prematurely");
                    exit(-1);
                }
            }
            if (recvline[0] == 'g' && recvline[1] == 'e' && recvline[2] == 't') {
                // download file from server
                // recvline: get:test.c,255
                downloadFile(recvline, sockfd);
            } else {
                fputs(recvline, stdout);
                fflush(stdout);
                // printf("%s\n",recvline);
            }
            fflush(stdout);
            bzero(recvline, sizeof(recvline));

        }

        if (FD_ISSET(fileno(fp), &rset)) {
            // input is readable

            if (read(fileno(fp), sendline, MAXLINE) == 0) {
                //EOF
                stdineof = 1;
                shutdown(sockfd, SHUT_WR);// half close, close write connection here (client -> server)
                printf("half closed\n");
                FD_CLR(fileno(fp), &rset);
                continue;

            }


            if (sendline[0] == 'p' && sendline[1] == 'u' && sendline[2] == 't') {
                // put 1.pdf or put /home/1.pdf
                char filename[128] = {0};
                int lastslash = 3;
                for (int j = 4; sendline[j] != '\n' && sendline[j] != '\000'; j++) {
                    //filename[j - 4] = sendline[j];
                    if (sendline[j] == '/') {
                        lastslash = j;
                    }
                }
                lastslash++;
                for (int j = lastslash; sendline[j] != '\n' && sendline[j] != '\000'; j++) {
                    filename[j - lastslash] = sendline[j];
                }
                //printf("filename = %s\n", filename);
                uploadFile(filename, sockfd);

            } else {
                write(sockfd, sendline, strlen(sendline));
            }
            bzero(sendline, sizeof(sendline));

        }

    }
}

#pragma clang diagnostic pop

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in serverAddr;
    if (argc != 3) {
        printf("wrong arguments! Please run ./client.out <ip> <port> \n");
        return -1;
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((uint16_t) atoi(argv[2]));// port
    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);
    connect(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    str_cli(stdin, sockfd);
    return 0;
}
