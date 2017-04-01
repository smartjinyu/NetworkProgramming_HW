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

#define MAXLINE 4096
#define LISTENQ 1024

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
int main(int argc, char **argv){
    int listenid,connfd,sockfd,i,maxi,maxfd;
    int nready,client[FD_SETSIZE];
    fd_set rset,allset;
    ssize_t n;
    char line[MAXLINE];
    struct sockaddr_in serverAddr,clientAddr;
    socklen_t clientLen;

    if(argc!=2){
        printf("wrong arguments! Please run ./server.out <port> \n");
        return -1;
    }

    listenid = socket(AF_INET,SOCK_STREAM,0);
    bzero(&serverAddr,sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);// for any interface on the server
    serverAddr.sin_port = htons((uint16_t)atoi(argv[1]));

    bind(listenid,(struct sockaddr *)&serverAddr,sizeof(serverAddr));
    // monitor the request on the port
    listen(listenid,LISTENQ);
    // LISTENQ the max length of the backlog (= complete + incomplete request queue)
    maxfd = listenid;
    maxi = -1;
    for(i=0;i<FD_SETSIZE;i++){
        client[i]=-1;
    }
    FD_ZERO(&allset);
    FD_SET(listenid,&allset);
    while(true){
        rset=allset;
        nready = select(maxfd+1,&rset,NULL,NULL,NULL);
        // number of ready descriptors
        if(FD_ISSET(listenid,&rset)){
            clientLen = sizeof(clientAddr);
            connfd = accept(listenid,(struct sockaddr *) &clientAddr,&clientLen);
            printf("New client connected, ip = %s, port = %d\n",
                   inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));
            // new client connection
            for(i=0;i<FD_SETSIZE;i++){
                if(client[i]<0){
                    // find the first available descriptor
                    client[i]=connfd;
                    break;
                }
            }
            if(i==FD_SETSIZE){
                printf("too many clients\n");
            }
            FD_SET(connfd,&allset);
            if(connfd > maxfd){
                maxfd = connfd;
            }
            if(i>maxi){
                maxi = i;
            }
            if(--nready<0){
                continue;// no more readable descriptors
            }

        }

        for(i=0;i<=maxi;i++){
            // check all clients for data
            if ((sockfd = client[i]) < 0) {
                continue; // skip empty client
            }
            if (FD_ISSET(sockfd, &rset)) {
                if ((n = read(sockfd, line, MAXLINE)) == 0) {
                    // connection closed by client
                    struct sockaddr_in terminatedAddr;
                    socklen_t len = sizeof(terminatedAddr);
                    getpeername(sockfd,(struct sockaddr *)&terminatedAddr,&len);
                    printf("Client terminated, ip = %s, port = %d\n",
                           inet_ntoa(terminatedAddr.sin_addr),ntohs(terminatedAddr.sin_port));

                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;

                } else {
                    write(sockfd, line, (size_t)n);
                    fputs(line,stdout);
                    for(int j=0;j<sizeof(line);j++){
                        line[j]=0;
                    }
                }
            }
            if (--nready < 0) {
                break; // no more readable descriptors
            }

        }
    }
}
#pragma clang diagnostic pop