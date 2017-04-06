
//
// Created by smartjinyu on 3/31/17.
//

#include <netinet/in.h>
#include <memory.h>
#include <arpa/inet.h>
#include <cstdio>
#include <unistd.h>
#include <cstdlib>

#define MAXLINE 2048

int max(int a, int b) {
    return a > b ? a : b;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void str_cli(FILE *fp,int sockfd){
    int maxfdp1,stdineof;
    fd_set rset;
    char sendline[MAXLINE+1],recvline[MAXLINE+1];
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
            if (recv(sockfd, recvline, MAXLINE,0) == 0) {
                if (stdineof == 1) {
                    //return; // normal termination
                } else {
                    printf("Client: Server terminated prematurely");
                    exit(-1);
                }
            }

            if(recvline[0]=='g' && recvline[1]=='e' && recvline[2]=='t'){
                // download file from server
                // recvline: get:test.c,255

                char filename[256],filesize[256];
                int file_size = 0;
                int j;
                for(j=4;recvline[j]!=',';j++){
                    filename[j-4]=recvline[j];
                }
                j++;
                int k;
                for(k=0;recvline[j]!=0;j++,k++){
                    filesize[k]=recvline[j];
                }
                file_size = atoi(filesize);

                FILE *recvFile;
                recvFile = fopen(filename,"w");
                ssize_t len;
                bzero(recvline,sizeof(recvline));
                int received_data = 0;
                bzero(recvline,sizeof(recvline));
                while(1){
                    len = read(sockfd,recvline,MAXLINE);
                    if(len<=0){
                        break;
                    }
                    received_data+=len;
                    fwrite(recvline,sizeof(char),len,recvFile);
                    if(received_data >= file_size){
                        break;
                    }
                }
                printf("Download Complete!\n");
                fclose(recvFile);

            }else{
                fputs(recvline, stdout);
                fflush(stdout);
                //printf("%s\n",recvline);
                bzero(recvline,sizeof(recvline));
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
                bzero(sendline,sizeof(sendline));

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
