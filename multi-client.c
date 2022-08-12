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

#define PORT 8002
int clientSocket;
void kill_server_sig_handler(int signum)
{
    if (signum == SIGINT)
    {
         char *buffer="exit";
        send(clientSocket, buffer, strlen(buffer), 0);
        close(clientSocket);
        printf("\n[-]Disconnected from server.\n");
        exit(1);
    }
}

int main()
{

    signal(SIGINT, kill_server_sig_handler);

    int ret;
    struct sockaddr_in serverAddr;
    char buffer[1024];

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        printf("[-]Error in connection.\n");
        exit(1);
    }
    printf("[+]Client Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        printf("[-]Error in connection.\n");
        exit(1);
    }
    printf("[+]Connected to Server.\n");

    while (1)
    {
        memset(buffer, 0, 1024);;
        printf("Client: ");
        fgets(buffer, 1024, stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        send(clientSocket, buffer, strlen(buffer), 0);
        if (strcmp(buffer, "exit") == 0)
        {
            close(clientSocket);
            printf("[-]Disconnected from server.\n");
            exit(1);
        }

        if (recv(clientSocket, buffer, 1024, 0) < 0)
        {
            printf("[-]Error in receiving data.\n");
        }
        else
        {
            if (strcmp(buffer, "CDUP") == 0)
            {
                char wd[100];
                printf("previous working directory : %s\n", getcwd(wd, 100));
                char resolved_path[1024];
                char *parent_dir = dirname(wd);
                chdir(parent_dir);
                printf("parent : %s\n", parent_dir);
            } else if (strstr(buffer, "CWD") != NULL)
            {
                int i=0;
                char* token = strtok(buffer, " ");
                char* array[2];
                char wd[100];

                while (token != NULL)
                {
                    array[i++] = token;
                    token = strtok (NULL, " ");
                }

                printf("%s\n", array[1]);
                printf("previous working dir : %s\n", getcwd(wd, 100));
                char resolved_path[1024];
                realpath(array[1], resolved_path);
                chdir(resolved_path);
                printf("current working directory : %s\n", getcwd(wd, 100));

            } else {
                printf("Server: %s\n", buffer);
            }
        }
    }

    return 0;
}