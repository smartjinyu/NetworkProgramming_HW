//
// Created by smartjinyu on 4/21/17.
//

#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <memory.h>
#include <cstdio>
#include <unistd.h>
#include <errno.h>

#define MAXLINE 4096
#define LISTENQ 1024
#define MAXCLIENTS 128

char clientName[256] = {0};// this client's name


struct clientInfo {
    char username[256] = {0};
    int sockfd = -1;
};

int max(int a, int b) {
    return a > b ? a : b;
}

void showHelpMenu() {
    printf("------------ Help Menu -------------\n");
    printf("help: show help menu\n");
    printf("post:<post content>: post a new article\n");
    printf("listposts: list posts catalog in the server\n");
    printf("readpost:<i>: read the post of index i in the server\n");
    printf("listclients: list all the clients online\n");
    printf("broadcast:<content>: broadcast to all online clients\n");
    printf("chatwith:<name>: chat with the client whose name is <name>\n");
    printf("chatserver: chat with server\n");
    printf("listchats: list chatters with this client \n");
    printf("endchat: end current chat \n");
    printf("------------ Help Menu -------------\n");
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main(int argc, char **argv) {
    int tcpfd, udpfd;
    int maxfdp1, stdineof;
    fd_set rset, allset;
    char sendline[MAXLINE] = {0}, recvline[MAXLINE] = {0};
    struct sockaddr_in serverAddr, CSclientAddr;
    socklen_t CSclientLen;
    int CSlistenfd = -1, CSconnectfd = -1, CSclientfd = -1;
    char chatclientName[256] = {0};
    // if this client is chat client, CSclientfd != -1
    // else remember to set CSclientfd = -1
    const int socketOpOn = 1;
    int nready;
    clientInfo CSclients[MAXCLIENTS];
    int maxCSclient = -1;
    int curChatter = -1;
    // curChatter corresponds to CSclients[i], -1 is chatting to server, -2 used as chat client

    ssize_t n;


    if (argc != 3) {
        fputs("wrong arguments! Please run ./client.out <ip> <port> \n", stderr);
        return -1;
    }

    // for TCP server socket
    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((uint16_t) atoi(argv[2]));// port
    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);
    setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &socketOpOn, sizeof(socketOpOn));

    if (connect(tcpfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        fprintf(stderr, "Connect to server failed, error = %s\n", strerror(errno));
        exit(-1);
    }


    showHelpMenu();
    printf("Please input the client name: ");
    fflush(stdout);

    // for UDP socket
    bzero(&serverAddr, sizeof(serverAddr));
    socklen_t len = sizeof(serverAddr);
    getsockname(tcpfd, (struct sockaddr *) &serverAddr, &len);
    // udp should not bind original port here
    // server send broadcasts to connected address, usually the port is different from argv[2]
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    bind(udpfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    // for chat Server
    CSlistenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    len = sizeof(serverAddr);
    getsockname(tcpfd, (struct sockaddr *) &serverAddr, &len);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    setsockopt(CSlistenfd, SOL_SOCKET, SO_REUSEADDR, &socketOpOn, sizeof(socketOpOn));
    bind(CSlistenfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    listen(CSlistenfd, LISTENQ);

    FD_ZERO(&rset);
    FD_ZERO(&allset);
    FD_SET(fileno(stdin), &allset); // for input
    FD_SET(tcpfd, &allset); // for socket
    FD_SET(udpfd, &allset);
    FD_SET(CSlistenfd, &allset);

    stdineof = 0; // use for test readable, 0:connect
    maxfdp1 = max(max(CSlistenfd, fileno(stdin)), max(tcpfd, udpfd));

    while (true) {
        rset = allset;
        if ((nready = select(maxfdp1 + 1, &rset, NULL, NULL, NULL)) < 0) {
            // number of ready descriptors
            if (errno == EINTR) {
                continue; // ignore this error
            } else {
                fprintf(stderr, "select error, error = %s\n", strerror(errno));
            }
        }

        if (FD_ISSET(tcpfd, &rset)) {
            // tcp socket is readable, communicate with server
            if (recv(tcpfd, recvline, MAXLINE, 0) == 0) {
                // printf("recv = 0\n");
                if (stdineof == 1) {
                    return 0; // normal termination
                } else {
                    printf("Client: Server terminated prematurely\n");
                    close(tcpfd);
                    FD_CLR(tcpfd, &allset);
                    continue;
                }
            }
            if (strncmp("chatserver:", recvline, 11) == 0) {
                // this client is chat client, connect to the chat server
                char chatterName[256] = {0};
                char chatterIp[256] = {0};
                char chatterPort[256] = {0};
                int j = 0, k = 0;
                for (j = 11; recvline[j] != ','; j++) {
                    chatterName[j - 11] = recvline[j];
                }
                j++;
                for (k = j; recvline[k] != ','; k++) {
                    chatterIp[k - j] = recvline[k];
                }
                k++;
                for (int l = k; recvline[l] != '\n' && recvline[l] != 0; l++) {
                    chatterPort[l - k] = recvline[l];
                }
                printf("Begin to chat with %s as client, server info: ip = %s, port= %d\n", chatterName, chatterIp,
                       atoi(chatterPort));
                CSclientfd = socket(AF_INET, SOCK_STREAM, 0);
                bzero(&serverAddr, sizeof(serverAddr));
                serverAddr.sin_family = AF_INET;
                serverAddr.sin_port = htons((uint16_t) atoi(chatterPort));
                inet_pton(AF_INET, chatterIp, &serverAddr.sin_addr);
                if (connect(CSclientfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
                    fprintf(stderr, "Connect to chat server failed, error = %s\n", strerror(errno));
                }
                // send name to chat server
                bzero(sendline, sizeof(sendline));
                sprintf(sendline, "name:%s", clientName);
                write(CSclientfd, sendline, sizeof(sendline));
                bzero(sendline, sizeof(sendline));

                FD_SET(CSclientfd, &allset);
                curChatter = -2;
                maxfdp1 = max(maxfdp1, CSclientfd);


            } else if (strncmp("chatclient:", recvline, 11) == 0) {
                // this client is chat server, here are client information
                char chatterName[256] = {0};
                char chatterIp[256] = {0};
                char chatterPort[256] = {0};
                int j = 0, k = 0;
                for (j = 11; recvline[j] != ','; j++) {
                    chatterName[j - 11] = recvline[j];
                }
                j++;
                for (k = j; recvline[k] != ','; k++) {
                    chatterIp[k - j] = recvline[k];
                }
                k++;
                for (int l = k; recvline[l] != '\n' && recvline[l] != 0; l++) {
                    chatterPort[l - k] = recvline[l];
                }
                printf("Begin to chat with %s as server, client info: ip = %s, port= %d\n", chatterName, chatterIp,
                       atoi(chatterPort));

            } else {
                fprintf(stdout, "%s", recvline);
                fflush(stdout);
            }
            bzero(recvline, sizeof(recvline));
        }


        if (FD_ISSET(udpfd, &rset)) {
            // udp socket is readable
            recvfrom(udpfd, recvline, MAXLINE, 0, NULL, NULL);
            fputs(recvline, stdout);
            fflush(stdout);
            bzero(recvline, sizeof(recvline));
        }


        if (FD_ISSET(fileno(stdin), &rset)) {
            // input is readable
            int sockfd = 0;
            if (curChatter == -1) {
                sockfd = tcpfd;
            } else if (curChatter == -2) {
                sockfd = CSclientfd;
            } else {
                sockfd = CSclients[curChatter].sockfd;
            }

            if (read(fileno(stdin), sendline, MAXLINE) == 0) {
                //EOF
                stdineof = 1;
                shutdown(tcpfd, SHUT_WR);// half close, close write connection here (client -> server)
                printf("half closed\n");
                FD_CLR(fileno(stdin), &allset);
                continue;
            }
            if (clientName[0] == 0 && curChatter == -1) {
                // clientName is not set
                char sendline0[MAXLINE] = {0}; // actual sendline this time
                strncpy(clientName, sendline, strlen(sendline) - 1); // last character of sendline is \n
                strcpy(sendline0, "name:");
                strcat(sendline0, sendline);
                write(sockfd, sendline0, strlen(sendline0));

            } else if (strncmp(sendline, "help", 4) == 0) {
                showHelpMenu();
            } else if (strncmp(sendline, "chatserver", 10) == 0) {
                printf("switch back to chat with server\n");
                curChatter = -1;
            } else if (strncmp(sendline, "listchats", 9) == 0) {
                // list current chatters
                if (curChatter == -1) {
                    printf("chat with server now\n");
                } else if (curChatter == -2) {
                    printf("chat with %s as chat client now\n", chatclientName);
                } else {
                    printf("chat with %s as chat server now\n", CSclients[curChatter].username);
                }
                printf("List all chatters:\n");
                if (FD_ISSET(tcpfd, &allset)) {
                    printf("server\n");
                }
                if (CSclientfd != -1) {
                    printf("%s\n", chatclientName);
                }
                for (int i = 0; i <= maxCSclient; i++) {
                    if (CSclients[i].sockfd != -1) {
                        printf("%s\n", CSclients[i].username);
                    }
                }
            } else if (strncmp(sendline, "endchat", 7) == 0) {
                // end current chat
                if (curChatter == -2) {
                    // chat as client
                    close(CSclientfd);
                    FD_CLR(CSclientfd, &allset);
                    CSclientfd = -1;
                    curChatter = -1; //communicate with server
                    printf("chat with %s ended, switch back to chat with server\n", chatclientName);
                    bzero(chatclientName, sizeof(chatclientName));
                } else if (curChatter == -1) {
                    close(tcpfd);
                    FD_CLR(tcpfd, &allset);
                    if (CSclientfd != -1) {
                        curChatter = -2;
                        printf("chat with server ended, switch back to chat with %s\n", chatclientName);
                    } else {
                        for (int i = 0; i <= maxCSclient; i++) {
                            if (CSclients[i].sockfd != -1) {
                                curChatter = i;
                                printf("chat with server ended, switch back to chat with %s\n", CSclients[i].username);
                                break;
                            }
                        }
                    }
                    if (curChatter == -1) {
                        printf("No connection now, client exit\n");
                        exit(0);
                    }
                } else {
                    close(CSclients[curChatter].sockfd);
                    FD_CLR(CSclients[curChatter].sockfd, &allset);
                    CSclients[curChatter].sockfd = -1;
                    printf("chat with %s ended, switch back to chat with server\n", CSclients[curChatter].username);
                    bzero(CSclients[curChatter].username, sizeof(CSclients[curChatter].username));
                    curChatter = -1;
                }
            } else if (strncmp(sendline, "chatwith:", 9) == 0) {
                char name[256] = {0};
                strncpy(name, sendline + 9, strlen(sendline) - 10);
                if (strcmp(name, clientName) == 0) {
                    printf("Please do not talk to yourself!\n");
                } else {
                    int i = 0;
                    for (i = 0; i <= maxCSclient; i++) {
                        if (strcmp(name, CSclients[i].username) == 0) {
                            break;
                        }
                    }
                    if (strcmp(name, chatclientName) == 0) {
                        i = -1;
                    }
                    if (i == maxCSclient + 1) {
                        // the client is not connected with me
                        write(tcpfd, sendline, strlen(sendline));
                    } else {
                        if (i == -1) {
                            printf("switch back to chat with %s as client\n", name);
                            curChatter = -2;
                        } else {
                            printf("switch back to chat with %s as server\n", name);
                            curChatter = i;
                        }
                    }
                }

            } else {
                // printf("curChatter = %d\n", curChatter);
                write(sockfd, sendline, strlen(sendline));
            }

            bzero(sendline, sizeof(sendline));
        }

        if (FD_ISSET(CSlistenfd, &rset)) {
            CSclientLen = sizeof(CSclientAddr);
            bzero(&CSclientAddr, sizeof(CSclientAddr));
            CSconnectfd = accept(CSlistenfd, (struct sockaddr *) &CSclientAddr, &CSclientLen);
            if (CSconnectfd < 0) {
                fprintf(stderr, "accept error, error = %s\n", strerror(errno));
                continue;
            }
            printf("New chat client connected, ip = %s, port = %d\n",
                   inet_ntoa(CSclientAddr.sin_addr), ntohs(CSclientAddr.sin_port));
            // send name to chat client
            bzero(sendline, sizeof(sendline));
            sprintf(sendline, "name:%s", clientName);
            write(CSconnectfd, sendline, sizeof(sendline));
            bzero(sendline, sizeof(sendline));

            // new client connection
            int i;
            for (i = 0; i < MAXCLIENTS; i++) {
                if (CSclients[i].sockfd < 0) {
                    // find the first available descriptor
                    CSclients[i].sockfd = CSconnectfd;
                    bzero(CSclients[i].username, sizeof(CSclients[i].username));
                    break;
                }
            }
            if (i == MAXCLIENTS) {
                printf("too many clients\n");
            }
            FD_SET(CSconnectfd, &allset);
            if (CSconnectfd > maxfdp1) {
                maxfdp1 = CSconnectfd;
            }
            if (i > maxCSclient) {
                maxCSclient = i;
            }
            curChatter = i;

            if (--nready <= 0) {
                continue;// no more readable descriptors
            }

        }

        if (FD_ISSET(CSclientfd, &rset)) {
            // receive information as chat client
            if ((n = read(CSclientfd, recvline, MAXLINE)) == 0) {
                // connection closed by client
                printf("Chat server terminated prematurely\n");
                close(CSclientfd);
                FD_CLR(CSclientfd, &allset);
                CSclientfd = -1;
                bzero(chatclientName, sizeof(chatclientName));
                curChatter = -1; //communicate with server
                continue;
            }
            if (strncmp(recvline, "name:", 5) == 0) {
                strncpy(chatclientName, recvline + 5, strlen(recvline) - 5);
            } else {
                fprintf(stdout, "%s: %s", chatclientName, recvline);
                fflush(stdout);
            }
            bzero(recvline, sizeof(recvline));
        }

        for (int i = 0; i <= maxCSclient; i++) {
            // receive information as chat server
            // check all clients for data
            int sockfd;
            if ((sockfd = CSclients[i].sockfd) < 0) {
                continue; // skip empty client
            }
            if (FD_ISSET(sockfd, &rset)) {
                if ((n = read(sockfd, recvline, MAXLINE)) == 0) {
                    // connection closed by client
                    struct sockaddr_in terminatedAddr;
                    bzero(&terminatedAddr, sizeof(terminatedAddr));
                    len = sizeof(terminatedAddr);
                    getpeername(sockfd, (struct sockaddr *) &terminatedAddr, &len);
                    printf("Client terminated, ip = %s, port = %d\n",
                           inet_ntoa(terminatedAddr.sin_addr), ntohs(terminatedAddr.sin_port));
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    CSclients[i].sockfd = -1;
                    bzero(CSclients[i].username, sizeof(CSclients[i].username));
                    continue;
                }
                if (strncmp(recvline, "name:", 5) == 0) {
                    strncpy(CSclients[i].username, recvline + 5, strlen(recvline) - 5);
                } else {
                    fprintf(stdout, "%s: %s", CSclients[i].username, recvline);
                    fflush(stdout);
                }
                bzero(recvline, sizeof(recvline));


                if (--nready <= 0) {
                    break; // no more readable descriptors
                }

            }
        }


    }

}

#pragma clang diagnostic pop
