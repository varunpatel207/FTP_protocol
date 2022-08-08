#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#define SERVER_PORT 9007

#define SOCKET_ERROR "Socket Error"
#define SOCKET_CREATE_SUCCESS "Socket created successfully"
#define SOCKET_BIND_ERROR "Socket Can't bind"
#define SOCKET_BIND_SUCCESS "Socket bind successful"
#define CLIENT_REQUEST_ERROR "Client request error"
#define CLIENT_REQUEST_SUCCESS "Client request success"
#define CONNECTION_ERROR "Could not connect to server"
#define CONNECTION_SUCCESS "Successfully connected to server"
#define MESSAGE_SENT "Message sent to server"
#define SOKCET_CLOSE_SUCCESS "Socket closed successfully"

// made global for signal
int socketFD;
void kill_client_sig_handler(int signum){
    if(signum == SIGINT) {
        close(socketFD);
        printf("%s\n", SOKCET_CLOSE_SUCCESS);
    }
}

int main() {
    // create client socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if(socketFD < 0) {
        printf("%s\n", SOCKET_ERROR);
        exit(0);
    } else {
        printf("%s\n", SOCKET_CREATE_SUCCESS);
    }

    // server socket information
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVER_PORT);

    // make connection to server
    if(connect(socketFD, (struct sockaddr *) &serveraddr, sizeof (serveraddr)) < 0) {
        printf("%s\n", CONNECTION_ERROR);
        exit(0);
    } else {
        printf("%s\n", CONNECTION_SUCCESS);
    }

    // wait for commands
    char buffer[10000] = "";
    char fileName[30];
    while(1) {
        bzero(buffer, sizeof(buffer));
        printf("\n> ");
        scanf("%[^\n]%*c" , buffer);
        write(socketFD , buffer, strlen(buffer));
        printf("\nMessage Sent\t:\t%s\n" , buffer);
        if ( strlen(buffer) > 7) {
            int len = strlen(buffer) - 5;
            strncpy(fileName , buffer + 5 , len);
            fileName[len] = '\0';
        }
        read(socketFD , buffer , sizeof(buffer));
        if(strcmp(buffer ,"\nConnection Termination\n") == 0 ) {
            printf("\nConnection Termination\n");
            break;
        }
        if( strcmp(buffer , "\n225 	Data connection open; no transfer in progress.\n") == 0) {
            printf("%s" , buffer);

            printf("\nFile Sending In Process\n");
            sleep(2);
            FILE *fPtr;
            fPtr = fopen(fileName, "r");
            if (fPtr == NULL) {
                printf("Cannot open file\t:\t%s \n", fileName);
            }
            else {
                char ch;
                bzero(buffer, sizeof(buffer));
                int i = 0;
                while(1) {
                    while ( ( (ch = fgetc(fPtr)) != EOF ) && ( strlen(buffer) - 1 != sizeof(buffer)) ) {
                        buffer[i] = ch;
                        i++;
                    }
                    buffer[i] = '\0';
                    if ( ch == EOF) {
                        write(socketFD , buffer, strlen(buffer));
                        break;
                    }
                    if ( strlen(buffer) == sizeof(buffer) ) {
                        write(socketFD , buffer, strlen(buffer));
                        bzero(buffer, sizeof(buffer));
                        i = 0;
                    }
                }
                printf("\nFile Sent Successfully\n");
                fclose(fPtr);
            }
        }
        else if ( strcmp(buffer , "\n226 	Data connection open; no transfer in progress.\n") == 0) {
            printf("%s" , buffer);
            FILE *fPtr;
            fPtr = fopen(fileName, "w");
            if (fPtr == NULL) {
                printf("Cannot open file\t:\t%s \n", fileName);
            }
            else {
                bzero(buffer, sizeof(buffer));
                while (1) {
                    read(socketFD , buffer , sizeof(buffer));
                    fputs(buffer , fPtr);
                    if ( strlen(buffer) != sizeof(buffer) ) {
                        break;
                    }
                    bzero(buffer, sizeof(buffer));
                }
                printf("\nFile Received Successfully\n");
                fclose(fPtr);
            }
        }
        else {
            printf("%s" , buffer);
        }
    }
    close(socketFD);

    return 0;
}
