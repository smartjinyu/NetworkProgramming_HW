//
// Created by smartjinyu on 3/13/17.
//


#include <netinet/in.h>
#include <strings.h>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#define MAXLINE 4096
#define LISTENQ 1024

int main(int argc, char **argv) {

    int listenid, connfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in client;
    socklen_t child;
    char buff[MAXLINE+1];
    time_t ticks;

    listenid = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);// for any interface on the server
    servaddr.sin_port = htons(1333);// port 13

    bind(listenid,(struct sockaddr*)&servaddr,sizeof(servaddr));
    // use bind to monitor the request on the port
    listen(listenid,LISTENQ);
    // LISTENQ the max length of the backlog (= complete + incomplete request queue)
    printf("waiting for connection...\n");
    for(;;){
        connfd = accept(listenid,(struct sockaddr*)&client,&child);

        // connfd = accept(listenid,(struct sockaddr*)NULL,NULL);
        // getpeername(connfd,(struct sockaddr*)&client,&child);
        // same as above
        printf("server = %s,port = %d\n",
               inet_ntoa(client.sin_addr),
               (int)ntohs(client.sin_port));
        ticks=time(NULL);
        snprintf(buff,sizeof(buff),"%.24s\r\n",ctime(&ticks));
        // snprintf Write formatted output to sized buffer
        write(connfd,buff,strlen(buff));
        close(connfd);
    }
}
