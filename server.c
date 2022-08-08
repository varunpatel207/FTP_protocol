#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#define SOCKET_PORT 9007

#define SOCKET_ERROR "Socket Error"
#define SOCKET_CREATE_SUCCESS "Socket created successfully"
#define SOCKET_BIND_ERROR "Socket Can't bind"
#define SOCKET_BIND_SUCCESS "Socket bind successful"
#define CLIENT_REQUEST_ERROR "Client request error"
#define CLIENT_REQUEST_SUCCESS "Client request success"
#define SOKCET_CLOSE_SUCCESS "Socket closed successfully"

// made global for kill or interrupt signal
int socketFD;
void kill_server_sig_handler(int signum){
    if(signum == SIGINT) {
        close(socketFD);
        printf("%s\n", SOKCET_CLOSE_SUCCESS);
    }
}

int main() {
    // register signal handler
    signal(SIGINT, kill_server_sig_handler);

    int bindSocket, acceptOutput;
    socklen_t clientMessage;

    // create socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    printf("socket fd: %d\n", socketFD);
    if(socketFD < 0) {
        printf("%s\n", SOCKET_ERROR);
        exit(0);
    } else {
        printf("%s\n", SOCKET_CREATE_SUCCESS);
    }

    // server socket address
    struct sockaddr_in serveraddr, clientaddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(SOCKET_PORT);

    // bind socket
    bindSocket = bind(socketFD, (struct sockaddr *)&serveraddr, sizeof (serveraddr));
    printf("bind socket value: %d\n", bindSocket);
    if(bindSocket < 0){
        printf("%s\n", SOCKET_BIND_ERROR);
        exit(0);
    } else {
        printf("%s\n", SOCKET_BIND_SUCCESS);
    };

    // listen client requests
    listen(socketFD, 5);
    // loop on request until broken

    char buffer[10000] = "";
    while(1) {
        // get client request
        clientMessage = sizeof (clientaddr);
        acceptOutput = accept(socketFD, (struct sockaddr *) &clientaddr, &clientMessage);
        read(acceptOutput , buffer , sizeof(buffer));
        printf("%s\n", buffer);
        if(acceptOutput < 0) {
            printf("%s\n", CLIENT_REQUEST_ERROR);
            exit(0);
        } else {
            printf("%s\n", CLIENT_REQUEST_SUCCESS);
        }

        // process client request
        pid_t pid;
        if((pid = fork()) == 0) {
            close(socketFD);

            // TODO: add main loop


            close(acceptOutput);
            exit(0);
        }
        close(acceptOutput);
    }
    close(socketFD);
    return 0;
}