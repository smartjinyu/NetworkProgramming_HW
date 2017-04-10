//
// Created by smartjinyu on 3/20/17.
//


#include <netinet/in.h>
#include <strings.h>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>


#define MAXLINE 4096
#define LISTENQ 1024


void str_echo(int sockfd){
    ssize_t n;
    char buf[MAXLINE];
    again:
        while((n=read(sockfd,buf,MAXLINE))>0){
            write(sockfd,buf,n);
        }
        if(n<0 && errno == EINTR){
            goto again; // ignore EINTR
        }else if(n<0){
            printf("str_echo:read error");
        }
}
void sig_child_wait(int signal){
    pid_t pid;
    int stat;
    pid = wait(&stat);
    printf("child %d terminated\n",pid);
    return;
}


void setKeepAlive(int sockfd) {
    int optval = 30;
    socklen_t optlen = sizeof(optval);
    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        printf("set Keep alive failed, error message = %s\n", strerror(errno));
    } else {
        printf("set Keep alive successfully!\n");
    }
}



void setReUseAddr(int sockfd) {
    int flag = 1; // 1 enabled
    socklen_t optlen = sizeof(flag);
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, optlen) < 0) {
        printf("set ReUseAddr failed, error message = %s\n", strerror(errno));
    } else {
        printf("set ReUseAddr successfully!\n");
    }
}

void setSoLinger(int sockfd) {
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;
    socklen_t optlen = sizeof(so_linger);
    if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &so_linger, optlen) < 0) {
        printf("set So_Linger, error message = %s\n", strerror(errno));
    } else {
        printf("set So_Linger successfully!\n");
    }
}


void sig_child_waitpid(int signal){
    pid_t pid;
    int stat;
    while((pid = waitpid(-1,&stat,WNOHANG))>0){
        printf("child %d terminated\n",pid);

    }
    //printf("child %d terminated\n",pid);
    return;
}

int main(int argc, char **argv) {

    int listenid, connfd;
    pid_t childpid;
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    socklen_t childlen;

    listenid = socket(AF_INET, SOCK_STREAM, 0);

    //setKeepAlive(listenid); // options will be inherited after connect
    //setReUseAddr(listenid);
    setSoLinger(listenid);


    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);// for any interface on the server
    servaddr.sin_port = htons(9877);// port 9877

    if(bind(listenid,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
        printf("bind error, error = %s\n",strerror(errno));
    }else{
        printf("bind successfully\n");
    }
    // use bind to monitor the request on the port
    listen(listenid,LISTENQ);
    // LISTENQ the max length of the backlog (= complete + incomplete request queue)
    for(;;){
        childlen = sizeof(clientaddr);
        connfd = accept(listenid,(struct sockaddr*)&clientaddr,&childlen);
        //signal(SIGCHLD,sig_child_wait);
        signal(SIGCHLD,sig_child_waitpid);
        if((childpid = fork())==0){
            printf("New client connected, server = %s,port = %d\n",
                   inet_ntoa(clientaddr.sin_addr),
                   (int)ntohs(clientaddr.sin_port));
            close(listenid);
            str_echo(connfd);
            exit(0);
        }
        close(connfd);
    }
}
