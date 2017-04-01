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
#include <wait.h>
#include <errno.h>

#define MAXLINE 4096
#define LISTENQ 1024
char curDir [FD_SETSIZE][128];
char defaultDir [128];

int listFiles(int sockfd) {
    char buf[MAXLINE];
    FILE *fp;

    bzero(&buf,sizeof(buf));

    if(getcwd(buf,sizeof(buf))!=NULL){
        write(sockfd,"List files in ",14);
        write(sockfd,buf,strlen(buf));
        write(sockfd,"\n",1);
    }else{
        write(sockfd,"Something went wrong...\n",24);
        return -1;
    }
    if ((fp = popen("ls -a", "r")) == NULL) {
        write(sockfd,"Something went wrong...\n",24);
        return -1;
    }

    bzero(&buf,sizeof(buf));
    while (fgets(buf, MAXLINE, fp) != NULL) {
        write(sockfd,buf,strlen(buf));
    }

    if(pclose(fp))  {
        return -1;
    }
    return 0;
}

int changeDir(char path[],int sockfd,int i){
    if(chdir(path)==0){
        bzero(&curDir[i],sizeof(curDir[i]));
        if(getcwd(curDir[i],sizeof(curDir[i]))!=NULL){
            write(sockfd,"Current path is ",16);
            write(sockfd,curDir[i],strlen(curDir[i]));
            write(sockfd,"\n",1);
        }else{
            write(sockfd,"Something went wrong...\n",24);
            return -1;
        }
        return 0;
    }else{
        write(sockfd,"Something went wrong...\n",24);
        return -1;
    }

}



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
    for(int j=0;j<FD_SETSIZE;j++){
        bzero(curDir[j],sizeof(curDir[j]));
    }
    bzero(defaultDir,sizeof(defaultDir));
    getcwd(defaultDir,sizeof(defaultDir));
    printf("default dir0 = %s \n",defaultDir);

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
                    strcpy(curDir[i],defaultDir);
                    printf("default dir = %s \n",curDir[i]);
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
                bzero(&line,sizeof(line));
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
                    bzero(curDir[i],sizeof(curDir[i]));

                } else {
                    //write(sockfd, line, (size_t)n);
                    printf("change to dir = %s \n",curDir[i]);
                    chdir(curDir[i]);

                    if(line[0]=='l' && line[1]=='s'){
                        listFiles(sockfd);
                    }else if(line[0]=='c' && line[1]=='d'){
                        char dir[MAXLINE];
                        bzero(&dir,sizeof(dir));
                        int j;
                        for(j = 3;line[j]!='\n' && line[j]!='\000';j++){
                            dir[j-3]=line[j];
                        }
                        changeDir(dir,sockfd,i);
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