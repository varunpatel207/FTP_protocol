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
#include <dirent.h>
#include <errno.h>

#define PORT 8007
#define COMMAND_OKAY "200 Command okay."
#define LOGIN_REQUIRED "530 Not logged in."
#define LOGIN_SUCCESS "230 User logged in, proceed."
#define ALREADY_LOGGED_IN "230 User already logged in, proceed."
#define OPEN_CONNECTION_NO_TRANSFER "225 Data connection open; no transfer in progress"
#define FILE_ACTION_COMPLETED "250 Requested file action okay, completed."
#define FILE_NOT_FOUND " 550 Requested action not taken. File unavailable."

int main_socket_fd;

int logged_in_array[100];
char rename_from[100][1024];
char rename_to[100][1024];
char list_string[5000];

// signal handler
void kill_server_sig_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTSTP)
    {

        char *message = "exit";
        shutdown(main_socket_fd, SHUT_RDWR);
        close(main_socket_fd);
        printf("\n[-]Socket closed successfully\n");
        exit(1);
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

// helper function to split input command
char *split_function(char *buffer) {
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

// create new directory
// first check if directory already exists with opendir function
// if not create the directory
char *mkd_function(char *buffer)
{
    char* dirname = split_function(buffer);
    char* message = "";

    DIR* dir = opendir(dirname);
    if (dir) {
        closedir(dir);
        message = concat(dirname, " directory name already exists.");
    } else {
        if (mkdir(dirname, 0755) == 0)
        {
            message = concat(dirname, " directory created successfully");
        }
    }

    return message;
}

// delete file
// check if file already exists with access function
char *dele_function(char *buffer)
{
    char *filename = split_function(buffer);
    char *message = "";

    if (access(filename, F_OK) == 0) {
        if (remove(filename) == 0)
            message = concat(filename, " file removed successfully.");
    } else {
        message = concat(filename, " file does not exists.");
    }

    return message;
}

// remove new directory
// check if directory already exists with opendir function
char *rmd_function(char *buffer)
{
    char *dirname = split_function(buffer);
    char* message = "";

    DIR* dir = opendir(dirname);
    if (dir) {
        closedir(dir);
        if (rmdir(dirname) == 0)
        {
            message = concat(dirname, " directory removed successfully.");
        }
    }else {
        message = concat(dirname, " directory does not exists.");
    }

    return message;
}

// array for list of directories and files
struct list_struct {
    char list_string[100][1024];
};


void list_function() {
    DIR *obj;
    struct dirent *dir;
    obj = opendir(".");

    struct list_struct l;
    int i = 0;
    if(obj) {
        while( (dir = readdir(obj) ) != NULL ) {
             strcpy(l.list_string[i], dir->d_name);
             i++;
        }
    }

    strcpy(list_string, "Files or directories in current directory");
    for (int k = 0; k < i; k++){
        strcat(list_string, l.list_string[k]);
        strcat(list_string, "\n");
    }

    closedir(obj);
}

int port_function(char *buffer)
{
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

    new_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (new_socket < 0)
    {
        printf("Data transfer socket create issue\n");
        exit(1);
    }
    else
    {
        printf("Data transfer socket is created.\n");
    }

    // address for binary socket
    memset(&new_serv_addr, '\0', sizeof(new_serv_addr));
    new_serv_addr.sin_family = AF_INET;
    new_serv_addr.sin_port = htons(new_port);
    new_serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // bind new socket
    int ret = bind(new_socket, (struct sockaddr *)&new_serv_addr, sizeof(new_serv_addr));
    if (ret < 0)
    {
        printf("Error in binding.\n");
        exit(1);
    }
    printf("Bind to port %d\n", new_port);

    // listen on new socket
    if (listen(new_socket, 100) == 0)
    {
        printf("Listening....\n");
        return new_socket;
    }
    else
    {
        printf("Error in binding.\n");
        return -1;
    }
}

// function to handle file upload from client
int stor_function(int sockfd, char *buffer)
{
    int n;
    FILE *fp;
    char *filename;
    char data[1024] = "\0";

    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " ");
    }
    filename = array[1];

    fp = fopen(filename, "w");
    if(fp == NULL) {
        return 0;
    }

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

// function to handle file append from client
// append to file content if the file already exists
// else create a file and write the content
int appe_function(int sockfd, char *buffer)
{
    int n;
    FILE *fp;
    char *filename;
    char data[1024] = "\0";

    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];
    char ch;

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " ");
    }
    filename = array[1];

    fp = fopen(filename, "a");
    if(fp == NULL) {
        fp = fopen(filename, "w");
    }

    while (1)
    {
        n = recv(sockfd, data, 1024, 0);
        if (n <= 0)
        {
            break;
            return 0;
        }
        fputs(data, fp);
        memset(data, 0, 1024);
    }
    fclose(fp);
    return 1;
}

// function to send file to client over data transfer port
int send_file(int sockfd, char *buffer)
{
    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " ");
    }

    char data[1024];
    FILE *fp;
    fp = fopen(array[1], "r");
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
    return 0;
}

// rename from (RNFR) function
// this command should be followed by rename to (RNTO) function
int rnfr_function(int sockfd, char *buffer)
{
    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " ");
    }

    char resolved_path[1024];
    realpath(array[1], resolved_path);
    if (access(resolved_path, F_OK) != -1)
    {
        strcpy(rename_from[sockfd], array[1]);
        return 1;
    }
    else
    {
        strcpy(rename_from[sockfd], "");
        return 0;
    }
}

// rename to (RNTO) function
// this command should be after RNFR function
int rnto_function(int sockfd, char *buffer)
{
    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " ");
    }

    char resolved_path[1024];
    realpath(rename_from[sockfd], resolved_path);
    if (access(resolved_path, F_OK) != -1)
    {
        strcpy(rename_to[sockfd], array[1]);
        rename(rename_from[sockfd], rename_to[sockfd]);
        return 1;
    }
    else
    {
        strcpy(rename_to[sockfd], "");
        return 0;
    }
}

int main(int argc, char *argv[])
{

    int ret;
    struct sockaddr_in server_addr;

    int newSocket;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    char buffer[1024];
    pid_t childpid;

    signal(SIGINT, kill_server_sig_handler);
    signal(SIGTSTP, kill_server_sig_handler);

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

    main_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (main_socket_fd < 0)
    {
        printf("Error in connection.\n");
        exit(1);
    }
    printf("Server Socket is created.\n");

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = bind(main_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        printf("Error in binding.\n");
        exit(1);
    }
    printf("Bind to port %d\n", PORT);

    if (listen(main_socket_fd, 100) == 0)
    {
        printf("Listening....\n");
    }
    else
    {
        printf("Error in binding.\n");
    }

    while (1)
    {
        newSocket = accept(main_socket_fd, (struct sockaddr *)&newAddr, &addr_size);
        if (newSocket < 0)
        {
            exit(1);
        }
        printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

        if ((childpid = fork()) == 0)
        {
            close(main_socket_fd);

            while (1)
            {
                int size = recv(newSocket, buffer, 1024, 0);
                int new_ft_socket;
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
                            char *message = mkd_function(buffer);
                            int sent = send(newSocket, message, strlen(message), 0);
                        }
                        if (strstr(buffer, "RMD") != NULL)
                        {
                            char *message = rmd_function(buffer);
                            int sent = send(newSocket, message, strlen(message), 0);
                        }
                        if (strstr(buffer, "DELE") != NULL)
                        {
                            char *message = dele_function(buffer);
                            int sent = send(newSocket, message, strlen(message), 0);
                        }
                        if (strstr(buffer, "PORT") != NULL)
                        {
                            int ft_socket_fd = port_function(buffer);
                            if (ft_socket_fd > -1)
                            {
                                char *message = "Socket success.";
                                int sent = send(newSocket, message, strlen(message), 0);

                                struct sockaddr_in new_ft_address;

                                socklen_t addr_size;
                                while (1)
                                {
                                    if (ft_socket_fd > -1)
                                    {
                                        new_ft_socket = accept(ft_socket_fd, (struct sockaddr *)&new_ft_address, &addr_size);
                                        if (new_ft_socket > 0)
                                        {
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        printf("Can not accept connection\n");
                                        break;
                                    }
                                }
                            }

                            else
                            {
                                char *message = "Socket not success.";
                                int sent = send(newSocket, message, strlen(message), 0);
                            }
                        }
                        if (strstr(buffer, "STOR") != NULL)
                        {
                            int res = stor_function(new_ft_socket, buffer);
                            char* message = res ? FILE_ACTION_COMPLETED : "File not found";
                            printf("%s\n", message);
                            close(new_ft_socket);
                            int sent = send(newSocket, message, strlen(message), 0);
                        }
                        if (strstr(buffer, "APPE") != NULL)
                        {
                            int res = appe_function(new_ft_socket, buffer);
                            printf("%s\n", FILE_ACTION_COMPLETED);
                            close(new_ft_socket);
                            int sent = send(newSocket, FILE_ACTION_COMPLETED, strlen(FILE_ACTION_COMPLETED), 0);
                        }
                        if (strstr(buffer, "RETR") != NULL)
                        {
                            int res = send_file(new_ft_socket, buffer);
                            printf("%s\n", FILE_ACTION_COMPLETED);
                            close(new_ft_socket);
                            int sent = send(newSocket, FILE_ACTION_COMPLETED, strlen(FILE_ACTION_COMPLETED), 0);
                        }
                        if (strstr(buffer, "RNFR") != NULL)
                        {
                            int result = rnfr_function(newSocket, buffer);
                            char *message = result ? "Please provide the new name of a file" : FILE_NOT_FOUND;
                            int sent = send(newSocket, message, strlen(message), 0);
                        }
                        if (strstr(buffer, "RNTO") != NULL)
                        {
                            // printf("in rnfr main\n");
                            int result = rnto_function(newSocket, buffer);
                            char *message = result ? "File renamed successfully." : FILE_NOT_FOUND;
                            int sent = send(newSocket, message, strlen(message), 0);
                        }
                        if (strcmp(buffer, "NOOP") == 0)
                        {
                            int sent = send(newSocket, COMMAND_OKAY, strlen(COMMAND_OKAY), 0);
                        }
                        if (strstr(buffer, "LIST") != NULL)
                        {
                            list_function();
                            int sent = send(newSocket, list_string, strlen(list_string), 0);
                            memset(list_string, 0, sizeof(list_string));
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