//
// Created by smartjinyu on 3/20/17.
//


#include <iostream>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define MAXLINE 4096
#define numConnections 1


int max(int a, int b) {
    return a > b ? a : b;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void str_cli(FILE *fp, int sockfd) {
    int maxfdp1, stdineof;
    fd_set rset;
    char sendline[MAXLINE], recvline[MAXLINE];
    FD_ZERO(&rset);
    stdineof = 0; // use for test readable, 0: connect
    while (true) {
        if(stdineof == 0){
            FD_SET(sockfd, &rset);// socket

        }
        FD_SET(fileno(fp), &rset);// input

        maxfdp1 = max(fileno(fp), sockfd) + 1;
        select(maxfdp1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &rset)) {
            // socket is readable
            if (read(sockfd, recvline, MAXLINE) == 0) {
                if (stdineof == 1) {
                    //return; // normal termination
                } else {
                    printf("Client: Server terminated prematurely");
                }
            }
            fputs(recvline, stdout);
            for(int j=0;j<sizeof(recvline);j++){
                recvline[j]=0;
            }

        }

        if (FD_ISSET(fileno(fp), &rset)) {
            // input is readable
            if (FD_ISSET(fileno(fp), &rset)) {
                // input is readable
                //if (fgets(sendline, MAXLINE, fp) == NULL) {
                fgets(sendline, MAXLINE, fp);
                if (sendline[0] == 'z') {
                    stdineof = 1;
                    shutdown(sockfd, SHUT_RD);// half close, close write connection here (client -> server)
                    printf("half closed\n");
                    FD_CLR(fileno(fp), &rset);
                    continue;
                }
                //}

                write(sockfd, sendline, strlen(sendline));
                for (int j = 0; j < sizeof(sendline); j++) {
                    sendline[j] = 0;
                }


                write(sockfd, sendline, strlen(sendline));
                for (int j = 0; j < sizeof(sendline); j++) {
                    sendline[j] = 0;
                }


            }


        }
    }
}
#pragma clang diagnostic pop

int main(int argc, char **argv) {


    int sockfd[numConnections];
    struct sockaddr_in servaddr;
    for (int i = 0; i < numConnections; i++) {
        sockfd[i] = socket(AF_INET, SOCK_STREAM, 0);
        // the third parameter Specifying a protocol of 0 causes socket() to use an unspecified default
        // protocol appropriate for the requested socket type (e.g., TCP/IP)
        // create a socket
    }


    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(9877); // use port 9877
    if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0) {
        // translate the ip address
        printf("inet_pton error for %s", "127.0.0.1");
        return -1;
    }

    for (int i = 0; i < numConnections; i++) {
        connect(sockfd[i], (struct sockaddr *) &servaddr, sizeof(servaddr));
    }
    str_cli(stdin, sockfd[0]);


    return 0;
}