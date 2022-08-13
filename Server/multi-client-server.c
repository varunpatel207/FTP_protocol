#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>

#define PORT 8004
#define LOGIN_REQUIRED "530 Not logged in."
#define LOGIN_SUCCESS "230 User logged in, proceed."
#define ALREADY_LOGGED_IN "230 User already logged in, proceed."
#define OPEN_CONNECTION_NO_TRANSFER "225 Data connection open; no transfer in progress"
#define FILE_ACTION_COMPLETED "250 Requested file action okay, completed."

int sockfd;

int logged_in_array[100];

void main_method(char *buffer);

void kill_server_sig_handler(int signum)
{
    if (signum == SIGINT)
    {
        close(sockfd);
        printf("\nSocket closed successfully\n");
    }
}

// helper method
char *concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

char *mkd_method(char *buffer)
{
    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " ");
    }

    if (mkdir(array[1], 0755) == 0)
    {
        return array[1];
    }

    return "";
}

char *dele_method(char *buffer)
{
    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " ");
    }

    if (remove(array[1]) == 0)
    {
        return array[1];
    }

    return "";
}

char *rmd_method(char *buffer)
{
    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " ");
    }

    if (rmdir(array[1]) == 0)
    {
        return array[1];
    }

    return "";
}

int port_function(char *buffer)
{
    printf("port_function\n");

    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];
    socklen_t addr_size;
    struct sockaddr_in newAddr;
    int new_ft_socket;
    int new_socket;
    struct sockaddr_in new_serv_addr;

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " ");
    }

    // array[1] = 2679
    int new_port = atoi(array[1]);

    printf("new port: %d\n", new_port);

    new_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (new_socket < 0)
    {
        printf("Socket create issue\n");
        exit(1);
    }
    else
    {
        printf("[+]Binary Socket is created.\n");
    }

    // address for binary socket
    memset(&new_serv_addr, '\0', sizeof(new_serv_addr));
    new_serv_addr.sin_family = AF_INET;
    new_serv_addr.sin_port = htons(new_port);
    new_serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // bind new socket
    int ret = bind(sockfd, (struct sockaddr *)&new_serv_addr, sizeof(new_serv_addr));
    if (ret < 0)
    {
        printf("Error in binding.\n");
        exit(1);
    }
    printf("Bind to port %d\n", new_port);

    // listen on new socket
    if (listen(new_socket, 100) == 0)
    {
        printf("Listening....111\n");
        return 1;
    }
    else
    {
        printf("Error in binding.\n");
        return 0;
    }
}

// void storFunction(char buffer[])
// {
//     int i = 0;
//     char *token = strtok(buffer, " ");
//     char *array[2];

//     while (token != NULL)
//     {
//         array[i++] = token;
//         token = strtok(NULL, " ");
//     }

//     char *fileName = array[1];
//     // Creating File
//     FILE *fPtr;
//     fPtr = fopen(fileName, "w");
//     if (fPtr == NULL)
//     {
//         strcat(buffer, fileName);
//         strcat(buffer, "\t:\t Does Not Exist\n");
//         return;
//     }
//     strcpy(buffer, "\n225 	Data connection open; no transfer in progress.\n");
//     fclose(fPtr);
// }

int write_file(int sockfd)
{
    int n;
    FILE *fp;
    char *filename = "recv.txt";
    char buffer[1024] = "\0";
    printf("in write file\n");

    fp = fopen(filename, "w+");
    // int fd1 = open("re1.txt", O_WRONLY | O_CREAT, 0777);
    while (1)
    {
        n = recv(sockfd, buffer, 1024, 0);
        printf("n : %s", buffer);
        if (n <= 0)
        {
            printf("in if break");
            break;
            return 0;
        }
        // printf("in here : ");
        // fputs(buffer, fp);
        // fflush(stdin);
        fprintf(fp, "%s", buffer);
        // write(fd1, buffer, 1024);
        memset(buffer, 0, 1024);
    }
    n = recv(sockfd, buffer, 1024, 0);
    fprintf(fp, "%s", buffer);
    // close(fd1);
    fclose(fp);
    return 1;
}

int main(int argc, char *argv[])
{

    int ret;
    struct sockaddr_in serverAddr;

    int newSocket;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    char buffer[1024];
    pid_t childpid;

    signal(SIGINT, kill_server_sig_handler);

    char home_dir[1024];
    if (argc == 3)
    {
        if (strcmp(argv[1], "-d") == 0)
        {
            realpath(argv[2], home_dir);
        }
    }
    else
    {
        getcwd(home_dir, 1024);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("[-]Error in connection.\n");
        exit(1);
    }
    printf("[+]Server Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        printf("[-]Error in binding.\n");
        exit(1);
    }
    printf("[+]Bind to port %d\n", PORT);

    if (listen(sockfd, 100) == 0)
    {
        printf("[+]Listening....\n");
    }
    else
    {
        printf("[-]Error in binding.\n");
    }

    while (1)
    {
        newSocket = accept(sockfd, (struct sockaddr *)&newAddr, &addr_size);
        if (newSocket < 0)
        {
            exit(1);
        }
        printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

        if ((childpid = fork()) == 0)
        {
            close(sockfd);

            while (1)
            {
                int size = recv(newSocket, buffer, 1024, 0);
                buffer[size] = '\0';
                if (strcmp(buffer, "exit") == 0)
                {
                    printf("Disconnected from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
                    break;
                }
                else
                {
                    printf("Client: %s\n", buffer);

                    if (logged_in_array[newSocket] == 1)
                    {
                        if (strcmp(buffer, "PWD") == 0)
                        {
                            int sent = send(newSocket, home_dir, strlen(home_dir), 0);
                        }
                        if (strcmp(buffer, "CDUP") == 0)
                        {
                            int sent = send(newSocket, buffer, strlen(buffer), 0);
                        }
                        if (strstr(buffer, "CWD") != NULL)
                        {
                            int sent = send(newSocket, buffer, strlen(buffer), 0);
                        }
                        if (strstr(buffer, "MKD") != NULL)
                        {
                            char *dirname = mkd_method(buffer);
                            char *message = concat(dirname, " created successfully");
                            int sent = send(newSocket, message, strlen(message), 0);
                        }
                        if (strstr(buffer, "RMD") != NULL)
                        {
                            char *dirname = rmd_method(buffer);
                            char *message = concat(dirname, " removed successfully");
                            int sent = send(newSocket, message, strlen(message), 0);
                        }
                        if (strstr(buffer, "DELE") != NULL)
                        {
                            char *dirname = dele_method(buffer);
                            char *message = concat(dirname, " removed successfully");
                            int sent = send(newSocket, message, strlen(message), 0);
                        }
                        int new_ft_socket;

                        if (strstr(buffer, "PORT") != NULL)
                        {
                            int ft_socket_fd = port_function(buffer);
                            if (ft_socket_fd == 1)
                            {
                                char *message = "Socket success.";
                                int sent = send(newSocket, message, strlen(message), 0);

                                struct sockaddr_in new_ft_address;

                                socklen_t addr_size;
                                while (1)
                                {
                                    printf("in while\n");
                                    new_ft_socket = accept(sockfd, (struct sockaddr *)&new_ft_address, &addr_size);
                                    if (new_ft_socket > 0)
                                    {
                                        break;
                                    }
                                }
                                printf("new_ft_socket %d\n", new_ft_socket);
                            }

                            else
                            {
                                char *message = "Socket not success.";
                                int sent = send(newSocket, message, strlen(message), 0);
                            }
                        }
                        if (strstr(buffer, "STOR") != NULL)
                        {
                            printf("in stor %d\n", new_ft_socket);
                            // int i = 0;
                            // char *token = strtok(buffer, " ");
                            // char *array[2];

                            // while (token != NULL)
                            // {
                            //     array[i++] = token;
                            //     token = strtok(NULL, " ");
                            // }
                            printf("before writing \n");
                            int res = write_file(new_ft_socket);
                            printf("[+]Data written in the file successfully.\n");
                            close(new_ft_socket);

                            int sent = send(newSocket, FILE_ACTION_COMPLETED, strlen(FILE_ACTION_COMPLETED), 0);
                        }
                    }
                    else
                    {
                        char *message = "";
                        if (strstr(buffer, "USER") != NULL)
                        {
                            logged_in_array[newSocket] = 1;
                            message = LOGIN_SUCCESS;
                        }
                        else
                        {
                            message = LOGIN_REQUIRED;
                        }
                        int sent = send(newSocket, message, strlen(message), 0);
                    }
                }

                bzero(buffer, sizeof(buffer));
            }
        }
    }

    close(newSocket);

    return 0;
}