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

#define PORT 8002
int sockfd;

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
char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

char* mkd_method(char *buffer) {
    int i=0;
    char * token = strtok(buffer, " ");
    char *array[2];

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok (NULL, " ");
    }

    if (mkdir(array[1], 0755) == 0){
        return array[1];
    }

    return "";
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

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

    if (listen(sockfd, 10) == 0)
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
                    printf("%s\n", buffer);
                    if(strcmp(buffer, "CWD") == 0) {
                        int sent = send(newSocket, home_dir, strlen(home_dir), 0);
                    }
                    if(strstr(buffer, "MKD") != NULL) {
                        char* dirname = mkd_method(buffer);
                        printf("dirname returned back %s", dirname);
                        char* message = concat(dirname, " created successfully");
                        int sent = send(newSocket, message, strlen(message), 0);
                    }
                    bzero(buffer, sizeof(buffer));
                }
            }
        }
    }

    close(newSocket);

    return 0;
}