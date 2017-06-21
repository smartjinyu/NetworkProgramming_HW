//
// Created by smartjinyu on 6/17/17.
//

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <map>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.


#define MAXLINE 4096
#define LISTENQ 1024
#define MAXCLIENTS 128
#define MAXNAME 128

struct clientInfo {
    int sockfd = -1;
    int type = -1;
    int user_id = -1;
    boost::uuids::uuid notiId;

    void clear() {
        sockfd = -1;
        type = -1;
        user_id = -1;
    }
};

struct messageInfo {
    int user_id = -1;
    char rawContent[MAXLINE] = {0}; // raw content will send to Windows client
    char mesgKey[MAXLINE] = {0}; // message key will send back to Android client
};

std::map<boost::uuids::uuid, messageInfo> messages;
clientInfo clients[MAXCLIENTS];

void setReUseAddr(int sockfd) {
    int flag = 1;
    socklen_t optlen = sizeof(flag);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, optlen);
}


void showClientTerminatedInfo(int index) {
    int sockfd = clients[index].sockfd;
    struct sockaddr_in terminatedAddr;
    bzero(&terminatedAddr, sizeof(terminatedAddr));
    socklen_t len = sizeof(terminatedAddr);
    getpeername(sockfd, (struct sockaddr *) &terminatedAddr, &len);
    printf("Client terminated, ip = %s, port = %d, type = %d, user_id = %d\n",
           inet_ntoa(terminatedAddr.sin_addr),
           ntohs(terminatedAddr.sin_port),
           clients[index].type,
           clients[index].user_id
    );
}

void sendMesgIdToWin(boost::uuids::uuid uuid, int user_id) {
    // send message id to PC main connection (type 101)
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i].sockfd != -1 && clients[i].user_id == user_id && clients[i].type == 101) {
            char sendline[MAXNAME] = {0};
            strcpy(sendline, boost::uuids::to_string(uuid).c_str());
            strcat(sendline, "#");
            write(clients[i].sockfd, sendline, strlen(sendline));
            printf("send mesg_id to win client, sockfd = %d, content = %s\n", clients[i].sockfd, sendline);
            // break; // only one windows client corresponding to a specific user_id
        }
    }

}

void sendActionToAndroid(boost::uuids::uuid uuid, int user_id, char *actionIndex) {
    // send action back to Android (type 202)
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i].sockfd != -1 && clients[i].user_id == user_id && clients[i].type == 202 &&
            clients[i].notiId == uuid) {
            char sendline[MAXLINE] = {0};
            strcpy(sendline, "key=");
            strcat(sendline, messages[uuid].mesgKey);
            strcat(sendline, ",actionindex=");
            strcat(sendline, actionIndex);
            strcat(sendline, "*=!#");
            write(clients[i].sockfd, sendline, strlen(sendline));
            printf("send action to Android client, sockfd = %d, content = %s\n", clients[i].sockfd, sendline);
        }
    }
    messages.erase(uuid); // this message has been done
}


void recvFromClient(int index) {
    ssize_t n;
    char recvline[MAXLINE] = {0}; /* every read buffer */
    char recvbuff[2 * MAXLINE] = {0}; /* store a full command */
    int sockfd = clients[index].sockfd;
    again:
    while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
        printf("%s\n", recvline);

        bzero(recvbuff, sizeof(recvbuff));
        strcpy(recvbuff, recvline);
        bzero(recvline, sizeof(recvline));
        int len = (int) strlen(recvbuff);
        while (!(recvbuff[len - 4] == '*' && recvbuff[len - 3] == '=' && recvbuff[len - 2] == '!' &&
                 recvbuff[len - 1] == '#')) {
            /* every message end with *=!# */
            /* a command isn't full */
            read(sockfd, recvline, MAXLINE);
            printf("%s\n", recvline);
            strcat(recvbuff, recvline);
            len = (int) strlen(recvbuff);
            bzero(recvline, sizeof(recvline));
        }
        printf("full command = %s\n", recvbuff);
        if (strncmp(recvbuff, "type=", 5) == 0) {
            std::string cmd(recvbuff);
            int pos0 = (int) cmd.find(",userid="); /* return the position of , */
            char typeStr[MAXNAME] = {0};
            strncpy(typeStr, recvbuff + 5, (size_t) pos0 - 5);
            int type = atoi(typeStr);
            // printf("connection type = %d\n",type);
            if (type == 202) {
                // Android send message info
                // type=202,userid=10001,content=key=0|com.android.messaging|0|com.android.messaging:sms:|10048
                // ##,notiname=Messaging,notitype=0,notititle=(650) 555-1212,noticontent=Nougat is sweet!
                // ,notiaction=Dismiss*=!#
                messageInfo message;
                int pos1 = (int) cmd.find(",content=");
                char user_idStr[MAXNAME] = {0};
                strncpy(user_idStr, recvbuff + pos0 + 8, (size_t) pos1 - pos0 - 8);
                int user_id = atoi(user_idStr);
                clients[index].user_id = user_id;
                clients[index].type = 202;
                message.user_id = user_id;
                // user id

                int pos2 = (int) cmd.find("##,notiname=");
                strncpy(message.mesgKey, recvbuff + pos1 + 13, (size_t) pos2 - pos1 - 13);
                // key

                int pos3 = (int) cmd.find("*=!#");
                strncpy(message.rawContent, recvbuff + pos2 + 3, (size_t) pos3 - pos2 - 3);
                // raw content

                boost::uuids::random_generator generator;
                boost::uuids::uuid uuid0 = generator();
                messages.insert(std::make_pair(uuid0, message));
                clients[index].notiId = uuid0;
                sendMesgIdToWin(uuid0, user_id);
                //break; // close connection
            } else if (type == 101) {
                // main connection from PC client
                // type=101,userid=10001*=!#
                int pos1 = (int) cmd.find("*=!#");
                char user_idStr[MAXNAME] = {0};
                strncpy(user_idStr, recvbuff + pos0 + 8, (size_t) pos1 - pos0 - 8);
                clients[index].type = 101;
                clients[index].user_id = atoi(user_idStr);
                // keep connection alve
            } else if (type == 102) {
                // PC get message details
                // type=102,userid=10001,mesg_id=xxx*=!#
                int pos1 = (int) cmd.find(",mesg_id=");
                char user_idStr[MAXNAME] = {0};
                strncpy(user_idStr, recvbuff + pos0 + 8, (size_t) pos1 - pos0 - 8);
                clients[index].type = 102;
                clients[index].user_id = atoi(user_idStr);
                //printf("user_id = %d\n", clients[index].user_id);
                // user id

                int pos2 = (int) cmd.find("*=!#");
                char uuidStr[MAXNAME] = {0};
                strncpy(uuidStr, recvbuff + pos1 + 9, (size_t) pos2 - pos1 - 9);
                boost::uuids::string_generator generator;
                boost::uuids::uuid uuid0 = generator(uuidStr);
                //printf("uuid = %s\n", boost::uuids::to_string(uuid0));
                messageInfo message = messages[uuid0];
                if (message.user_id == atoi(user_idStr)) {
                    char sendline[MAXLINE] = {0};
                    strcpy(sendline, message.rawContent);
                    strcat(sendline, "*=!#");
                    write(clients[index].sockfd, sendline, strlen(sendline));
                } else {
                    fprintf(stderr, "Request message details belonging to other users!\n");
                }
                break; // close connection
            } else if (type == 103) {
                // PC send message action back
                // type=103,userid=xxxx,mesg_id=xxx,actionindex=i,notitype=-1,notiinput=xxx*=!#
                int pos1 = (int) cmd.find(",mesg_id=");
                char user_idStr[MAXNAME] = {0};
                strncpy(user_idStr, recvbuff + pos0 + 8, (size_t) pos1 - pos0 - 8);
                int user_id = atoi(user_idStr);
                clients[index].type = 103;
                clients[index].user_id = user_id;
                //printf("user_id = %d\n", user_id);
                // user id

                int pos2 = (int) cmd.find(",actionindex=");
                char uuidStr[MAXNAME] = {0};
                strncpy(uuidStr, recvbuff + pos1 + 9, (size_t) pos2 - pos1 - 9);
                boost::uuids::string_generator generator;
                boost::uuids::uuid uuid0 = generator(uuidStr);

                char otherInfo[MAXLINE] = {0};
                strcpy(otherInfo, recvbuff + pos2); //,actionindex=i,notitype=-1,notiinput=xxx*=!#
                //printf("uuid = %s\n", boost::uuids::to_string(uuid0));
                // mesg_id
                /*
                int pos3 = (int) cmd.find(",notitype=");
                char actionIndexStr[MAXNAME] = {0};
                strncpy(actionIndexStr, recvbuff + pos2 + 13, (size_t) pos3 - pos2 - 13);
                // printf("action index = %s\n", actionIndexStr);
                // action index

                int pos4 = (int) cmd.find(",notiinput=");
                char notiInput[256]={0};
                strncpy(notiInput, recvbuff + pos3 + 10, (size_t) pos4 - pos3 - 10);
                // notiInput

                int pos5 = (int) cmd.find("*=!#");
                */
                // send to Android Client
                for (int i = 0; i < MAXCLIENTS; i++) {
                    if (clients[i].sockfd != -1 && clients[i].user_id == user_id && clients[i].type == 202 &&
                        clients[i].notiId == uuid0) {
                        char sendline[MAXLINE] = {0};
                        strcpy(sendline, "key=");
                        strcat(sendline, messages[uuid0].mesgKey);
                        strcat(sendline, otherInfo);
                        write(clients[i].sockfd, sendline, strlen(sendline));
                        printf("send action to Android client, sockfd = %d, content = %s\n", clients[i].sockfd,
                               sendline);
                    }
                }
                messages.erase(uuid0); // this message has been done

                break;
            }

        } else {
            break; // unexpected message
        }

    }

    if (n < 0 && errno == EINTR) {
        goto again; // ignore EINTR
    } else if (n < 0) {
        fprintf(stderr, "servRecv:read error");
        return;
    }
}

void *servFunc(void *arg) {
    int index = *((int *) arg);
    free(arg);
    pthread_detach(pthread_self());
    int connfd = clients[index].sockfd;
    recvFromClient(index);

    // thread terminated
    showClientTerminatedInfo(index);
    clients[index].clear();

    close(connfd);
    return (NULL);
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main(int argc, char **argv) {
    int listenfd, connfd;
    pthread_t tid;
    socklen_t clientAddrLen;
    struct sockaddr_in servaddr, clientaddr;


    if (argc != 2) {
        fprintf(stderr, "Wrong arguments! Please run ./Server <port>\n");
        exit(-1);
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    setReUseAddr(listenfd);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((uint16_t) atoi(argv[1]));

    bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    listen(listenfd, LISTENQ);
    messages.clear();
    printf("This is the server of Android Notification Mirror, final project of Network Programming\n");

    for (;;) {
        bzero(&clientaddr, sizeof(clientaddr));
        clientAddrLen = sizeof(clientaddr);
        connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientAddrLen);
        int i = 0;
        for (i = 0; i < MAXCLIENTS; i++) {
            if (clients[i].sockfd == -1) {
                clients[i].sockfd = connfd;
                break;
            }
        }
        printf("New client connected, index = %d, ip = %s, port = %d\n",
               i, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        int *arg = (int *) malloc(sizeof(*arg));
        *arg = i;
        pthread_create(&tid, NULL, &servFunc, arg); /* pass by value*/
    }
}

#pragma clang diagnostic pop

