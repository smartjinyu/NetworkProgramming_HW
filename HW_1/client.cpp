
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
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        write(sockfd, "Failed to open file:", 20);
        write(sockfd, strerror(errno), strlen(strerror(errno)));
        fprintf(stderr, "Failed to open file %s: %s\n", filename, strerror(errno));
        return -1;
    }

    struct stat stat_buf;// file size to be sent
    fstat(fd, &stat_buf);
    off_t offset = 0;


    char header[256] = {0};
    sprintf(header, "put:%s,%d", filename, (int) stat_buf.st_size);
    write(sockfd, header, sizeof(header));

    int remain_data = (int) stat_buf.st_size;
    ssize_t len;
    while ((len = sendfile(sockfd, fd, &offset, MAXLINE)) > 0) {
        remain_data -= len;
    }
    write(sockfd, "ok\n", 3);
    close(fd);
    return 0;
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
                printf("filename = %s, filesize = %d\n",filename,file_size);
                if(remove(filename)!= 0){
                    perror( "Error deleting file\n" );

                } else {
                    puts( "File successfully deleted\n" );
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
                        printf("len = %d, received = %d\n",(int)len,received_data);
                        fwrite(recvline, sizeof(char), (size_t)len, recvFile);
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
                fputs(recvline, stdout);
                fflush(stdout);
                // printf("%s\n",recvline);
            }
            fflush(stdout);
            bzero(recvline, sizeof(recvline));

        }

        if (FD_ISSET(fileno(fp), &rset)) {
            // input is readable

            if (read(fileno(fp), sendline, MAXLINE) == NULL) {
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
                printf("filename = %s\n", filename);
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
