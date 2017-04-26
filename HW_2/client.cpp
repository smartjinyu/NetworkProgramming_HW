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

char clientName[256] = {0};

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
    printf("------------ Help Menu -------------\n");
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main(int argc, char **argv) {
    int tcpfd, udpfd;
    int maxfdp1, stdineof;
    fd_set rset;
    char sendline[MAXLINE] = {0}, recvline[MAXLINE] = {0};
    struct sockaddr_in serverAddr;
    if (argc != 3) {
        fputs("wrong arguments! Please run ./client.out <ip> <port> \n", stderr);
        return -1;
    }

    // for TCP socket
    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((uint16_t) atoi(argv[2]));// port
    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);

    if (connect(tcpfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        fprintf(stderr, "Connect to server failed, error = %s\n", strerror(errno));
        exit(-1);
    }

    FD_ZERO(&rset);
    stdineof = 0; // use for test readable, 0:connect

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


    while (true) {
        if (stdineof == 0) {
            FD_SET(fileno(stdin), &rset); // for input
        }
        FD_SET(tcpfd, &rset); // for socket
        FD_SET(udpfd, &rset);
        maxfdp1 = max(fileno(stdin), max(tcpfd, udpfd)) + 1;
        select(maxfdp1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(tcpfd, &rset)) {
            // tcp socket is readable
            if (recv(tcpfd, recvline, MAXLINE, 0) == 0) {
                printf("recv = 0\n");
                if (stdineof == 1) {
                    return 0; // normal termination
                } else {
                    printf("Client: Server terminated prematurely");
                    exit(-1);
                }
            }
            fputs(recvline, stdout);
            fflush(stdout);
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

            if (read(fileno(stdin), sendline, MAXLINE) == 0) {
                //EOF
                stdineof = 1;
                shutdown(tcpfd, SHUT_WR);// half close, close write connection here (client -> server)
                printf("half closed\n");
                FD_CLR(fileno(stdin), &rset);
                continue;
            }
            if (clientName[0] == 0) {
                // clientName is not set
                char sendline0[MAXLINE] = {0}; // actual sendline this time
                strncpy(clientName, sendline, strlen(sendline) - 1); // last character of sendline is \n
                strcpy(sendline0, "name:");
                strcat(sendline0, sendline);
                write(tcpfd, sendline0, strlen(sendline0));

            } else {
                if (strncmp(sendline, "help", 4) == 0) {
                    showHelpMenu();
                } else {
                    write(tcpfd, sendline, strlen(sendline));
                }

            }

            bzero(sendline, sizeof(sendline));
        }


    }

}

#pragma clang diagnostic pop
