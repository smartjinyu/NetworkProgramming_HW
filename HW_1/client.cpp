
//
// Created by smartjinyu on 3/31/17.
//

#include <netinet/in.h>
#include <memory.h>
#include <arpa/inet.h>
#include <cstdio>
#include <unistd.h>
#include <cstdlib>

#define MAXLINE 4096

int max(int a, int b) {
    return a > b ? a : b;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void str_cli(FILE *fp,int sockfd){
    int maxfdp1,stdineof;
    fd_set rset;
    char sendline[MAXLINE],recvline[MAXLINE];
    FD_ZERO(&rset);
    stdineof = 0; // use for test readable, 0:connect
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
                    exit(-1);
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

int main(int argc,char **argv){
    int sockfd;
    struct sockaddr_in serverAddr;
    if(argc!=3){
        printf("wrong arguments! Please run ./client.out <ip> <port> \n");
        return -1;
    }
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    bzero(&serverAddr,sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((uint16_t)atoi(argv[2]));// port
    inet_pton(AF_INET,argv[1],&serverAddr.sin_addr);
    connect(sockfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr));
    str_cli(stdin,sockfd);
    return 0;
}
