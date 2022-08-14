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

#define PORT 8007
#define EXIT "exit"
#define LOGIN_REQUIRED "530 Not logged in."
#define LOGIN_SUCCESS "230 User logged in, proceed."
#define ALREADY_LOGGED_IN "230 User already logged in, proceed."
#define OPEN_CONNECTION_NO_TRANSFER "225 Data connection open; no transfer in progress."
#define FILE_ACTION_COMPLETED "250 Requested file action okay, completed."
#define SOCKET_CLOSED "426 Connection closed."
#define CONNECTION_ERROR "425 Can't open data connection."
#define COMMAND_OK "200 Command okay."
#define SYNTAX_ERROR "500 Syntax error, command unrecognized."

int client_socket;


// signal handler
void kill_server_sig_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTSTP)
    {
        send(client_socket, EXIT, strlen(EXIT), 0);
        close(client_socket);
        printf("%s\n", SOCKET_CLOSED);
        exit(1);
    }
}

// helper method
char *split_command(char *buffer) {
    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " ");
    }
    return array[1];
}

// function to send file to client over data transfer port
void send_file(char *filename, int sockfd)
{
    char data[1024];
    FILE *fp;
    fp = fopen(filename, "r");
    while (fgets(data, 1024, fp) != NULL)
    {
        if (send(sockfd, data, sizeof(data), 0) == -1)
        {
            perror("[-]Error in sending file.");
            exit(1);
        }
        bzero(data, 1024);
    }
    fclose(fp);
    fflush(stdout);
}

// function to handle write downloaded file from server over data transfer socket
int write_file(int sockfd, char *buffer)
{
    int n;
    FILE *fp;
    char *filename = split_command(buffer);
    char data[1024] = "\0";

    fp = fopen(filename, "w");
    while (1)
    {
        n = recv(sockfd, data, 1024, 0);
        if (n <= 0)
        {
            break;
            return 0;
        }
        fprintf(fp, "%s", data);
        memset(data, 0, 1024);
    }
    fclose(fp);
    return 1;
}

int main()
{
    signal(SIGINT, kill_server_sig_handler);
    signal(SIGTSTP, kill_server_sig_handler);

    int ret;
    struct sockaddr_in server_addr;
    char buffer[1024];
    char response_buffer[1024];

    // create client socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        printf("%s\n", "Socket connection error");
        exit(1);
    }
    printf("%s\n", "Socket connected successfully");

    // socket connect for file transfer
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    // connect client socket to server socket
    ret = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        printf("%s\n", CONNECTION_ERROR);
        exit(1);
    }
    printf("%s\n", OPEN_CONNECTION_NO_TRANSFER);

    while (1)
    {
        int new_ret = -1;
        int new_client_socket;
        memset(buffer, 0, 1024);
        memset(response_buffer, 0, 1024);
        printf("Client: ");
        fgets(buffer, 1024, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        // code to handle STOR command
        // send file data to server on data transfer port
        if (strstr(buffer, "STOR") != NULL)
        {
            fflush(stdout);
            char temp_buffer[1024];
            strcpy(temp_buffer, buffer);
            char file_real_path[1024];

            char* filename = split_command(temp_buffer);

            realpath(filename, file_real_path);

            send_file(file_real_path, new_client_socket);
            close(new_client_socket);
        }

        // code to handle APPE command
        // send file data to server on data transfer port
        if (strstr(buffer, "APPE") != NULL)
        {
            fflush(stdout);
            char temp_buffer[1024];
            char file_real_path[1024];

            strcpy(temp_buffer, buffer);

            char *filename = split_command(temp_buffer);
            realpath(filename, file_real_path);

            send_file(file_real_path, new_client_socket);
            close(new_client_socket);
        }

        // send command to client
        send(client_socket, buffer, strlen(buffer), 0);

        // close client socket on exit
        if (strcmp(buffer, "exit") == 0)
        {
            close(client_socket);
            printf("Disconnected from server.\n");
            exit(1);
        }

        int n_read = 0;
        if ((n_read = recv(client_socket, response_buffer, 1024, 0)) < 0)
        {
            printf("Error in receiving data.\n");
        }
        else
        {
            // move to parent directory of CWD
            if (strcmp(response_buffer, "CDUP") == 0)
            {
                char wd[100];
                printf("previous working directory : %s\n", getcwd(wd, 100));
                char resolved_path[1024];
                char *parent_dir = dirname(wd);
                chdir(parent_dir);
                printf("parent : %s\n", parent_dir);
            }

            // CWD on client side
            else if (strstr(response_buffer, "CWD") != NULL)
            {
                char wd[100];
                char *dirname = split_command(buffer);

                printf("%s\n", dirname);
                printf("previous working dir : %s\n", getcwd(wd, 100));
                char dir_real_path[1024];
                realpath(dirname, dir_real_path);
                chdir(dir_real_path);
                printf("current working directory : %s\n", getcwd(wd, 100));
            }

            // handle PORT command reponse
            else if (strcmp(response_buffer, "Socket success.") == 0)
            {
                char wd[100];
                char* port_number = split_command(buffer);
                int new_port = atoi(port_number);
                new_client_socket = socket(AF_INET, SOCK_STREAM, 0);
                server_addr.sin_port = htons(new_port);
                new_ret = connect(new_client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
                printf("%s\n", OPEN_CONNECTION_NO_TRANSFER);
            }

            // STOR command response
            else if (strstr(buffer, "STOR") != NULL)
            {
                printf("Server: %s\n", response_buffer);
                printf("File uploaded successfully.\n");
                close(new_client_socket);
            }

            // APPE command response
            else if (strstr(buffer, "APPE") != NULL)
            {
                printf("Server: %s\n", response_buffer);
                printf("File appended successfully.\n");
                close(new_client_socket);
            }

            // RETR command response
            else if (strstr(buffer, "RETR") != NULL)
            {
                fflush(stdout);
                char wd[100];
                char temp_buffer[1024];
                strcpy(temp_buffer, buffer);

                char* prev_file_name = split_command(temp_buffer);
                char file_name[1024];
                realpath(prev_file_name, file_name);

                write_file(new_client_socket, buffer);

                close(new_client_socket);
                printf("Server: %s\n", response_buffer);
                printf("File downloaded successfully.\n");
            }
            else
            {
                printf("Server: %s\n", response_buffer);
            }
        }
    }

    return 0;
}