#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <libgen.h>
#include <fcntl.h>

#define PORT 8004
int clientSocket;

void kill_server_sig_handler(int signum)
{
    if (signum == SIGINT)
    {
        char *message = "exit";
        send(clientSocket, message, strlen(message), 0);
        close(clientSocket);
        printf("\n[-]Disconnected from server.\n");
        exit(1);
    }
}

// void file_send(int sockFD, char *buffer, char *fileName)
// {
//     printf("buffer : %s", buffer);

//     printf("\nFile Sending In Process\n");
//     sleep(2);
//     FILE *fPtr;
//     fPtr = fopen(fileName, "r");
//     if (fPtr == NULL)
//     {
//         printf("Cannot open file\t:\t%s \n", fileName);
//     }
//     else
//     {
//         char ch;
//         bzero(buffer, sizeof(buffer));
//         int i = 0;
//         while (1)
//         {
//             while (((ch = fgetc(fPtr)) != EOF) && (strlen(buffer) - 1 != sizeof(buffer)))
//             {
//                 buffer[i] = ch;
//                 i++;
//             }
//             buffer[i] = '\0';
//             if (ch == EOF)
//             {
//                 write(sockFD, buffer, strlen(buffer));
//                 break;
//             }
//             if (strlen(buffer) == sizeof(buffer))
//             {
//                 write(sockFD, buffer, strlen(buffer));
//                 bzero(buffer, sizeof(buffer));
//                 i = 0;
//             }
//         }
//         printf("\nFile Sent Successfully\n");
//         fclose(fPtr);
//     }
// }

void send_file(char *filename, int sockfd)
{
    // int n;
    char data[1024];
    FILE *fp;
    fp = fopen(filename, "r");
    printf("socket : %d", sockfd);
    while (fgets(data, 1024, fp) != NULL)
    {
        printf("data : %s", data);
        if (send(sockfd, data, sizeof(data), 0) == -1)
        {
            perror("[-]Error in sending file.");
            exit(1);
        }
        bzero(data, 1024);
    }
    fclose(fp);
    fflush(stdout);
    // int fd1 = open(filename, O_RDONLY);
    // int n = 0;
    // while (n = read(fd1, data, 1024) > 0)
    // {
    //     printf("data : %s", data);
    //     if (send(sockfd, data, n, 0) == -1)
    //     {
    //         perror("[-]Error in sending file.");
    //         exit(1);
    //     }
    //     memset(data, 0, 1024);
    // }
}

int main()
{

    signal(SIGINT, kill_server_sig_handler);

    int ret;
    struct sockaddr_in serverAddr;
    char buffer[1024];
    char response_buffer[1024];

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        printf("[-]Error in connection.\n");
        exit(1);
    }
    printf("[+]Client Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // socket connect for file transfer

    serverAddr.sin_port = htons(PORT);
    ret = connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        printf("[-]Error in connection.\n");
        exit(1);
    }
    printf("[+]Connected to Server.\n");

    while (1)
    {
        int new_ret = -1;
        int new_client_socket;
        memset(buffer, 0, 1024);
        memset(response_buffer, 0, 1024);
        printf("Client: ");
        fgets(buffer, 1024, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strstr(buffer, "STOR") != NULL)
        {
            printf("in stor : %s\n", buffer);
            fflush(stdout);
            int i = 0;
            char *token = strtok(buffer, " ");
            char *array[2];
            char wd[100];

            while (token != NULL)
            {
                array[i++] = token;
                token = strtok(NULL, " ");
            }

            char file_name[1024];
            realpath(array[1], file_name);
            printf("filename : %s\n", file_name);
            // FILE *fp = fopen(file_name, "r");
            // if (fp == NULL)
            // {
            //     perror("[-]Error in reading file.");
            //     exit(1);
            // }

            send_file(file_name, new_client_socket);
            printf("[+]File data sent successfully.\n");
        }

        send(clientSocket, buffer, strlen(buffer), 0);
        if (strcmp(buffer, "exit") == 0)
        {
            close(clientSocket);
            printf("[-]Disconnected from server.\n");
            exit(1);
        }

        if (recv(clientSocket, response_buffer, 1024, 0) < 0)
        {
            printf("[-]Error in receiving data.\n");
        }
        else
        {
            if (strcmp(response_buffer, "CDUP") == 0)
            {
                char wd[100];
                printf("previous working directory : %s\n", getcwd(wd, 100));
                char resolved_path[1024];
                char *parent_dir = dirname(wd);
                chdir(parent_dir);
                printf("parent : %s\n", parent_dir);
            }
            else if (strstr(response_buffer, "CWD") != NULL)
            {
                int i = 0;
                char *token = strtok(buffer, " ");
                char *array[2];
                char wd[100];

                while (token != NULL)
                {
                    array[i++] = token;
                    token = strtok(NULL, " ");
                }

                printf("%s\n", array[1]);
                printf("previous working dir : %s\n", getcwd(wd, 100));
                char resolved_path[1024];
                realpath(array[1], resolved_path);
                chdir(resolved_path);
                printf("current working directory : %s\n", getcwd(wd, 100));
            }
            else if (strcmp(response_buffer, "Socket success.") == 0)
            {
                int i = 0;
                char *token = strtok(buffer, " ");
                char *array[2];
                char wd[100];

                while (token != NULL)
                {
                    array[i++] = token;
                    token = strtok(NULL, " ");
                }

                int new_port = atoi(array[1]);

                new_client_socket = socket(AF_INET, SOCK_STREAM, 0);
                printf("new socket : %d", new_client_socket);
                serverAddr.sin_port = htons(new_port);
                new_ret = connect(new_client_socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
            }
            else if (strstr(buffer, "STOR") != NULL)
            {
                printf("response : %s\n", response_buffer);
                printf("buffer : %s\n", buffer);
            }
            // else if (strstr(buffer, "STOR") != NULL)
            // {
            //     int i = 0;
            //     char *token = strtok(buffer, " ");
            //     char *array[2];
            //     char wd[100];

            //     while (token != NULL)
            //     {
            //         array[i++] = token;
            //         token = strtok(NULL, " ");
            //     }

            //     char *file_name;
            //     realpath(array[1], file_name);
            //     printf("filename : %s\n", file_name);

            //     if (new_ret > -1)
            //     {
            //         file_send(new_client_socket, response_buffer, file_name);
            //     }
            // }
            else
            {
                printf("Server: %s\n", response_buffer);
            }
        }
    }

    return 0;
}