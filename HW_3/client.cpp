//
// Created by smartjinyu on 5/19/17.
//

#include <netinet/in.h>
#include <memory.h>
#include <cstdio>
#include <cstdlib>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
#include <vector>

#define MAXLINE 4096
#define MAXNAMELEN 256
#define LISTENQ 1024
#define MAXCLIENTS 128


struct clientInfo {
    char username[MAXNAMELEN] = {0};
    int sockfd = -1;
    std::vector<std::string> filenames;
};

clientInfo clients[MAXCLIENTS];

int sockfdClient; /* global for both threads to access */
FILE *fpStdin;

char clientName[MAXNAMELEN] = {0};

void *clientInput(void *);
void *servListen(void *);
void listfiles(int sockfd) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(".")) != NULL) {
        /* print all the files and directories within directory */
        char sendline[MAXLINE] = {0};
        strcpy(sendline, "listMyfiles:<");
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                // regular file
                strcat(sendline, ent->d_name);
                strcat(sendline, ",");
            }
        }
        sendline[strlen(sendline) - 1] = '>';
        write(sockfd, sendline, strlen(sendline));
        closedir(dir);
    } else {
        /* could not open directory */
        perror("");
        return;
    }

}

void str_cli(FILE *fp_arg) {
    char recvline[MAXLINE] = {0};
    pthread_t tid;

    fpStdin = fp_arg;

    pthread_create(&tid, NULL, clientInput, NULL);
    while (read(sockfdClient, recvline, MAXLINE) > 0) {
        fputs(recvline, stdout);
        bzero(recvline, sizeof(recvline));
    }
}

void showHelpMenu() {
    printf("------------ Help Menu -------------\n");
    printf("help: show help menu\n");
    printf("listclients: list all the clients online\n");
    printf("listfiles:<index> list files of clients of index (-1 on the server)\n");
    printf("------------ Help Menu -------------\n");
}

int main(int argc, char **argv) {
    struct sockaddr_in servaddr;
    const int socketOpOn =1;
    if (argc != 3) {
        fprintf(stderr, "Wrong arguments! Please run ./Client <ip> <port>\n");
        exit(-1);
    }

    // client socket
    bzero(&servaddr, sizeof(servaddr));
    sockfdClient = socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons((uint16_t) atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    setsockopt(sockfdClient, SOL_SOCKET, SO_REUSEADDR, &socketOpOn, sizeof(socketOpOn));


    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        // translate the ip address
        fprintf(stderr, "inet_pton error for %s", argv[1]);
        return -1;
    }
    if (connect(sockfdClient, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "Connect failed, error message = %s\n", strerror(errno));
    }
    showHelpMenu();
    printf("Please input the client name: ");
    fflush(stdout);


    pthread_t tid;
    pthread_create(&tid, NULL, servListen, NULL);

    listfiles(sockfdClient);
    str_cli(stdin);
    return 0;
}

void *clientInput(void *arg) {
    char sendline[MAXLINE] = {0};

    while (fgets(sendline, MAXLINE, fpStdin) != NULL) {
        if (clientName[0] == 0) {
            // name unset
            strncpy(clientName, sendline, strlen(sendline) - 1); // last character is \n
            bzero(sendline, sizeof(sendline));
            strcpy(sendline, "name:");
            strcat(sendline, clientName);
            strcat(sendline, "\n");
        }
        if (strncmp(sendline, "help", 4) == 0) {
            showHelpMenu();
            continue;
        }
        write(sockfdClient, sendline, strlen(sendline));
        bzero(sendline, sizeof(sendline));

    }
    return (NULL);
}



void showClientTerminatedInfo(int sockfd) {
    struct sockaddr_in terminatedAddr;
    bzero(&terminatedAddr, sizeof(terminatedAddr));
    socklen_t len = sizeof(terminatedAddr);
    getpeername(sockfd, (struct sockaddr *) &terminatedAddr, &len);
    printf("Client terminated, ip = %s, port = %d\n",
           inet_ntoa(terminatedAddr.sin_addr), ntohs(terminatedAddr.sin_port));
}


void str_echo(int sockfd, int index) {
    ssize_t n;
    char recvline[MAXLINE] = {0};
    again:
    while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
        fputs(recvline, stdout);
        write(sockfd, recvline, strlen(recvline));
        bzero(recvline, sizeof(recvline));
    }

    if (n < 0 && errno == EINTR) {
        goto again; // ignore EINTR
    } else if (n < 0) {
        fprintf(stderr, "str_echo:read error");
        return;
    }
}


void *doit(void *arg) {
    int index;
    index = *((int *) arg);
    free(arg);
    pthread_detach(pthread_self());
    int connfd = clients[index].sockfd;
    // printf("New client connected, index = %d, sockfdClient = %d\n", index, connfd);
    str_echo(connfd, index);

    // thread terminated
    showClientTerminatedInfo(clients[index].sockfd);
    clients[index].sockfd = -1;
    bzero(clients[index].username, sizeof(clients[index].username));
    clients[index].filenames.clear();
    close(connfd);
    return (NULL);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void *servListen(void *arg){
    int CSlistenfd = -1, CSconnectfd = -1, CSclientfd = -1;
    struct sockaddr_in serverAddr, CSclientAddr;
    const int socketOpOn = 1;
    socklen_t clientAddrLen;
    pthread_t tid;


    CSlistenfd = socket(AF_INET, SOCK_STREAM, 0);
    printf("CSlistenfd = %d\n",CSlistenfd);
    bzero(&serverAddr, sizeof(serverAddr));
    socklen_t len = sizeof(serverAddr);
    getsockname(sockfdClient, (struct sockaddr *) &serverAddr, &len);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    setsockopt(CSlistenfd, SOL_SOCKET, SO_REUSEADDR, &socketOpOn, sizeof(socketOpOn));
    int result = bind(CSlistenfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if(result==-1){
        printf("Error when bind, error = %s\n",strerror(errno));
    }

    result = listen(CSlistenfd, LISTENQ);
    if(result==-1){
        printf("Error when listen, error = %s\n",strerror(errno));
    }
    for (;;) {
        bzero(&CSclientAddr, sizeof(CSclientAddr));
        clientAddrLen = sizeof(CSclientAddr);
        CSconnectfd = accept(CSlistenfd, (struct sockaddr *) &CSclientAddr, &clientAddrLen);
        int i = 0;
        for (i = 0; i < MAXCLIENTS; i++) {
            if (clients[i].sockfd == -1) {
                clients[i].sockfd = CSconnectfd;
                break;
            }
        }
        printf("New client connected, index = %d, ip = %s, port = %d\n",
               i, inet_ntoa(CSclientAddr.sin_addr), ntohs(CSclientAddr.sin_port));

        int *argcc = (int *) malloc(sizeof(*argcc));
        *argcc = i;
        pthread_create(&tid, NULL, &doit, argcc); /* pass by value*/
    }

}
#pragma clang diagnostic pop