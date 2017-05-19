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

#define MAXLINE 4096
#define MAXNAMELEN 256

static int sockfd; /* global for both threads to access */
static FILE *fp;

char clientName[MAXNAMELEN] = {0};

void *copyto(void *);

void str_cli(FILE *fp_arg, int sockfd_arg) {
    char recvline[MAXLINE] = {0};
    pthread_t tid;

    sockfd = sockfd_arg;
    fp = fp_arg;

    pthread_create(&tid, NULL, copyto, NULL);
    while (read(sockfd, recvline, MAXLINE) > 0) {
        fputs(recvline, stdout);
        bzero(recvline, sizeof(recvline));
    }
}

void showHelpMenu() {
    printf("------------ Help Menu -------------\n");
    printf("help: show help menu\n");
    printf("listclients: list all the clients online\n");
    printf("------------ Help Menu -------------\n");
}

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (argc != 3) {
        fprintf(stderr, "Wrong arguments! Please run ./Client <ip> <port>\n");
        exit(-1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons((uint16_t) atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        // translate the ip address
        fprintf(stderr, "inet_pton error for %s", argv[1]);
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "Connect failed, error message = %s\n", strerror(errno));
    }
    showHelpMenu();
    printf("Please input the client name: ");
    fflush(stdout);
    str_cli(stdin, sockfd);
    return 0;
}

void *copyto(void *arg) {
    char sendline[MAXLINE] = {0};

    while (fgets(sendline, MAXLINE, fp) != NULL) {
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
        write(sockfd, sendline, strlen(sendline));
        bzero(sendline, sizeof(sendline));

    }
    return (NULL);
}