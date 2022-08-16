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

// handler function to handle CTRL-C and CTRL-Z signals
void kill_server_sig_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTSTP) // if the incoming signal is CTR-C (SIGNINT) or CTRL-Z (SIGTSTP) then
    {

        char *message = "exit";
        shutdown(main_socket_fd, SHUT_RDWR); // shutdown the sockets
        close(main_socket_fd);               // close the sockets
        printf("\n[-]Socket closed successfully\n");
        exit(1); // exit from the program
    }
}

// function to concat strings
char *concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // initializing the variable using malloc to store both string and +1 for the null-terminator

    strcpy(result, s1); // copying the first string into result variable
    strcat(result, s2); // concating second string into result variable
    return result;      // return the result
}

// function to split the command entered by user from whitespace (" ")
char *split_function(char *buffer)
{
    int i = 0;
    char *token = strtok(buffer, " "); // using strtok method to split the buffer from token i.e. whitespace
    char *array[2];                    // defining array variable to store the command and argument

    while (token != NULL) // while the string still has whitespaces
    {
        array[i++] = token;        // assigning the splitted strings to array
        token = strtok(NULL, " "); // continuing splitting the string from whitespace
    }

    return array[1]; // return the first argument of command which is stored in array[1]
}

// function to create new directory
char *mkd_function(char *buffer)
{
    char *dirname = split_function(buffer); // get the directory name by splitting the buffer/command
    char *message = "";

    DIR *dir = opendir(dirname); // open the directory
    if (dir)                     // check if directory already exists with opendir function
    {
        closedir(dir);                                                // close the directory
        message = concat(dirname, " directory name already exists."); // and print this message
    }
    else // if not then create the directory
    {
        if (mkdir(dirname, 0755) == 0) // create the new directory using mkdir function
        {
            message = concat(dirname, " directory created successfully"); // concat the directory name and message to print
        }
    }

    return message; // return the message
}

// function to delete file
char *dele_function(char *buffer)
{
    char *filename = split_function(buffer); // get the filename by splitting the buffer/command
    char *message = "";

    if (access(filename, F_OK) == 0) // check if file already exists with access function
    {
        if (remove(filename) == 0)                                     // if the file is removed successfully then
            message = concat(filename, " file removed successfully."); // concat the file name and message to print
    }
    else
    {
        message = concat(filename, " file does not exists."); // concat the file name and message to print
    }

    return message; // return the message
}

// function to remove directory
char *rmd_function(char *buffer)
{
    char *dirname = split_function(buffer); // get the directory name by splitting the buffer/command
    char *message = "";

    DIR *dir = opendir(dirname); // open the directory
    if (dir)                     // check if directory already exists with opendir function
    {
        closedir(dir);           // close the directory
        if (rmdir(dirname) == 0) // if the directory is removed successfully then
        {
            message = concat(dirname, " directory removed successfully."); // concat the directory name and message to print
        }
    }
    else
    {
        message = concat(dirname, " directory does not exists."); // concat the directory name and message to print
    }

    return message; // return the message
}

// structure to store list of directories
struct list_struct
{
    char list_string[100][1024];
};

// function to list all the directories in directory
void list_function()
{
    DIR *obj;
    struct dirent *dir;
    obj = opendir("."); // open the current directory

    struct list_struct l;
    int i = 0;
    if (obj) // if directory is opened then
    {
        while ((dir = readdir(obj)) != NULL) // read the directory
        {
            strcpy(l.list_string[i], dir->d_name); // copy the directory name into the 2d array
            i++;                                   // increase the counter
        }
    }

    strcpy(list_string, "Files or directories in current directory");
    for (int k = 0; k < i; k++)
    {
        strcat(list_string, l.list_string[k]); // concating the elements of array into single string one by one
        strcat(list_string, "\n");             // adding \n at every line
    }

    closedir(obj); // closing the opened directory
}

// function to create the new socket for data/file transfer
int port_function(char *buffer)
{
    int i = 0;
    char *token = strtok(buffer, " "); // using strtok method to split the buffer from token i.e. whitespace
    char *array[2];                    // defining array variable to store the command and argument
    socklen_t addr_size;
    struct sockaddr_in newAddr;
    int new_ft_socket;
    int new_socket;
    struct sockaddr_in new_serv_addr;

    while (token != NULL) // the string still has whitespaces
    {
        array[i++] = token;        // assigning the splitted strings to array
        token = strtok(NULL, " "); // continuing splitting the string from whitespace
    }

    int new_port = atoi(array[1]); // convert the port number into integer

    new_socket = socket(AF_INET, SOCK_STREAM, 0); // create the new socket to communicate with server
    if (new_socket < 0)                           // if the socket creation is not successful then
    {
        printf("Data transfer socket create issue\n"); // print this message
        exit(1);                                       // exit from the program
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
    int ret = bind(new_socket, (struct sockaddr *)&new_serv_addr, sizeof(new_serv_addr)); //  bind the new socket to new port
    if (ret < 0)                                                                          // if binding is not successful then
    {
        printf("Error in binding.\n"); // print this message
        exit(1);                       // exit from the program
    }
    printf("Bind to port %d\n", new_port);

    // listen on new socket
    if (listen(new_socket, 100) == 0) // listen the coming connection on this socket
    {
        printf("Listening....\n");
        return new_socket; // return the fd of new socket
    }
    else
    {
        printf("Error in binding.\n"); // otherwise print this message
        return -1;                     // and return -1
    }
}

// function to handle file upload from client
int stor_function(int sockfd, char *buffer)
{
    int n;
    FILE *fp;
    char *filename;
    char data[1024] = "\0"; // initializing the array with 0s to store data coming from client

    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " ");
    }
    filename = array[1];

    fp = fopen(filename, "w"); // opening the file in write mode
    if (fp == NULL)            // if file is not opened
    {
        return 0; // the return from the function
    }

    while (1) // infinite while loop
    {
        n = recv(sockfd, data, 1024, 0); // receiving the data from client into the blocks of 1024 bytes in data variable
        if (n <= 0)                      // if nothing is getting returned from client then
        {
            break; // break the loop
            return 0;
        }
        fprintf(fp, "%s", data); // print that data into the file
        memset(data, 0, 1024);   // clearing the data variable
    }
    fclose(fp); // close the file writing stream
    return 1;
}

// function to handle file append from client

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

    fp = fopen(filename, "a"); // append to file content if the file already exists
    if (fp == NULL)            // else create a file and write the content
    {
        fp = fopen(filename, "w");
    }

    while (1)
    {
        n = recv(sockfd, data, 1024, 0); // receiving the data from client into the blocks of 1024 bytes in data variable
        if (n <= 0)                      // if nothing is getting returned from client then
        {
            break;
            return 0;
        }
        fputs(data, fp);       // print that data into the file
        memset(data, 0, 1024); // clearing the data variable
    }
    fclose(fp); // close the file writing stream
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

    char data[1024]; // defining the array of 1024 bytes to store the data of files
    FILE *fp;
    fp = fopen(array[1], "r");            // opening the file in read mode
    while (fgets(data, 1024, fp) != NULL) // read the file in the blocks of 1024 bytes and store it in data variable
    {
        if (send(sockfd, data, sizeof(data), 0) == -1) // send the data to server
        {
            perror("[-]Error in sending file.");
            exit(1); // if sending is unsuccessful then exit the program
        }
        bzero(data, 1024); // clear the data variable
    }
    fclose(fp); // close the file reading stream
    fflush(stdout);
    return 0;
}

// function to store old filename
// this command should be followed by rename to (RNTO) function
int rnfr_function(int sockfd, char *buffer)
{
    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " "); // getting the old filename in array[1]
    }

    char resolved_path[1024];
    realpath(array[1], resolved_path);     // getting the realpath of the mentioned filename
    if (access(resolved_path, F_OK) != -1) // if file exists then
    {
        strcpy(rename_from[sockfd], array[1]); // copy the name of the file into rename from array for sock fd
        return 1;
    }
    else
    {
        strcpy(rename_from[sockfd], ""); // otherwise store empty string for that particular socket fd
        return 0;
    }
}

//  function to store new filename and renaming the file
// this command should be after RNFR function
int rnto_function(int sockfd, char *buffer)
{
    int i = 0;
    char *token = strtok(buffer, " ");
    char *array[2];

    while (token != NULL)
    {
        array[i++] = token;
        token = strtok(NULL, " "); // getting the new filename in array[1]
    }

    char resolved_path[1024];
    realpath(rename_from[sockfd], resolved_path); // getting the realpath of the mentioned filename
    if (access(resolved_path, F_OK) != -1)        // if file exists then
    {
        strcpy(rename_to[sockfd], array[1]);            // copy the name of the file into rename from array for sock fd
        rename(rename_from[sockfd], rename_to[sockfd]); // rename the file with nw filename
        return 1;
    }
    else
    {
        strcpy(rename_to[sockfd], ""); // otherwise store empty string for that particular socket fd
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

    signal(SIGINT, kill_server_sig_handler);  // installing the handler for SIGINT (CTRL-C) signal
    signal(SIGTSTP, kill_server_sig_handler); // installing the handler for SIGTSTP (CTRL-Z) signal

    char home_dir[1024];
    if (argc == 3) // if arguments' count is 3
    {
        if (strcmp(argv[1], "-d") == 0) // and second argument is -d then
        {
            realpath(argv[2], home_dir); // save the realpath of directory mentioned in argument 3 to home_dir variable
        }
    }
    else
    {
        getcwd(home_dir, 1024); // else save the path of current directory to home_dir variable
    }

    main_socket_fd = socket(AF_INET, SOCK_STREAM, 0); // creating main socket to establish a connection
    if (main_socket_fd < 0)                           // if socket is not created then
    {
        printf("Error in connection.\n"); // print this message
        exit(1);                          // exit from the program
    }
    printf("Server Socket is created.\n");

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = bind(main_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)); // bind the main socket to mentined port number
    if (ret < 0)                                                                      // if binding is not successful then
    {
        printf("Error in binding.\n"); // print this message
        exit(1);                       // and exit form the program
    }
    printf("Bind to port %d\n", PORT);

    if (listen(main_socket_fd, 100) == 0) // listen to the main socket for incoming messages from client
    {
        printf("Listening....\n");
    }
    else
    {
        printf("Error in binding.\n");
    }

    while (1) // infinite while loop
    {
        newSocket = accept(main_socket_fd, (struct sockaddr *)&newAddr, &addr_size); // accept the new connection on the main socket
        if (newSocket < 0)
        {
            exit(1);
        }
        printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

        if ((childpid = fork()) == 0) // fork the process to maintain multiple clients and if it is child then,
        {
            close(main_socket_fd);

            while (1) // infinite while loop
            {
                int size = recv(newSocket, buffer, 1024, 0); // receive the data coming from client in 1024 bytes of block into buffer variable
                int new_ft_socket;
                buffer[size] = '\0';             // add \0 at the end of the buffer
                if (strcmp(buffer, "exit") == 0) // if the command is exit then
                {
                    printf("Disconnected from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
                    break; // break the while loop
                }
                else // otherwise
                {
                    printf("Client: %s\n", buffer);
                    if (logged_in_array[newSocket] == 1) // if user is logged in by typing USER command first, then and only then perform following commands
                    {
                        if (strcmp(buffer, "PWD") == 0) // if command is PWD
                        {
                            int sent = send(newSocket, home_dir, strlen(home_dir), 0); // send the home_dir variable to client which has current working directory
                        }
                        if (strcmp(buffer, "CDUP") == 0) // if command is CDUP
                        {
                            int sent = send(newSocket, buffer, strlen(buffer), 0); // send the same command to client to perform CDUP on client side
                        }
                        if (strstr(buffer, "CWD") != NULL) // if command is CWD
                        {
                            int sent = send(newSocket, buffer, strlen(buffer), 0); // send the same command to client to perform CWD on client side
                        }
                        if (strstr(buffer, "MKD") != NULL) // if command is MKD
                        {
                            char *message = mkd_function(buffer);                    // call the mkd_function to create new directory
                            int sent = send(newSocket, message, strlen(message), 0); // send the approriate message to client
                        }
                        if (strstr(buffer, "RMD") != NULL) // if command is RMD
                        {
                            char *message = rmd_function(buffer);                    // call the rmd_function to create new directory
                            int sent = send(newSocket, message, strlen(message), 0); // send the approriate message to client
                        }
                        if (strstr(buffer, "DELE") != NULL) // if command is DELE
                        {
                            char *message = dele_function(buffer);                   // call the dele_function to create new directory
                            int sent = send(newSocket, message, strlen(message), 0); // send the approriate message to client
                        }
                        if (strstr(buffer, "PORT") != NULL) // if command is DELE
                        {
                            int ft_socket_fd = port_function(buffer); // call the port_function to create new socket for data/file transfer
                            if (ft_socket_fd > -1)                    // if it sends non negative value then
                            {
                                char *message = "Socket success.";
                                int sent = send(newSocket, message, strlen(message), 0); // send this message to client

                                struct sockaddr_in new_ft_address;

                                socklen_t addr_size;
                                while (1) // infinite while loop
                                {
                                    if (ft_socket_fd > -1)
                                    {
                                        new_ft_socket = accept(ft_socket_fd, (struct sockaddr *)&new_ft_address, &addr_size); // accept the connection on new socket for file transfer
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
                                char *message = "Socket not success.";                   // if it is not successful then
                                int sent = send(newSocket, message, strlen(message), 0); // send the message to client
                            }
                        }
                        if (strstr(buffer, "STOR") != NULL) // if the command is STOR
                        {
                            int res = stor_function(new_ft_socket, buffer);                 // call stor_function to store the file sent from client on server side
                            char *message = res ? FILE_ACTION_COMPLETED : "File not found"; // store the approriate message
                            printf("%s\n", message);
                            close(new_ft_socket);                                    // close the socket created for file transfer
                            int sent = send(newSocket, message, strlen(message), 0); // send the message to client
                        }
                        if (strstr(buffer, "APPE") != NULL) // if the command is APPE
                        {
                            int res = appe_function(new_ft_socket, buffer); // call appe_function to store the file sent from client on server side
                            printf("%s\n", FILE_ACTION_COMPLETED);
                            close(new_ft_socket);                                                                // close the socket created for file transfer
                            int sent = send(newSocket, FILE_ACTION_COMPLETED, strlen(FILE_ACTION_COMPLETED), 0); // send the message to client
                        }
                        if (strstr(buffer, "RETR") != NULL) // if the command is RETR
                        {
                            int res = send_file(new_ft_socket, buffer); // call send_function to send the file from server to client
                            printf("%s\n", FILE_ACTION_COMPLETED);
                            close(new_ft_socket);                                                                // close the socket created for file transfer
                            int sent = send(newSocket, FILE_ACTION_COMPLETED, strlen(FILE_ACTION_COMPLETED), 0); // send the message to client
                        }
                        if (strstr(buffer, "RNFR") != NULL) // if the command is RNFR
                        {
                            int result = rnfr_function(newSocket, buffer); // call rnfr_function to store the old filename
                            char *message = result ? "Please provide the new name of a file" : FILE_NOT_FOUND;
                            int sent = send(newSocket, message, strlen(message), 0); // send the message to client
                        }
                        if (strstr(buffer, "RNTO") != NULL) // if the command is RNTO
                        {
                            // printf("in rnfr main\n");
                            int result = rnto_function(newSocket, buffer); // call rnto_function to store the new filename and to rename the file
                            char *message = result ? "File renamed successfully." : FILE_NOT_FOUND;
                            int sent = send(newSocket, message, strlen(message), 0); // send the message to client
                        }
                        if (strcmp(buffer, "NOOP") == 0) // if the command is NOOP
                        {
                            int sent = send(newSocket, COMMAND_OKAY, strlen(COMMAND_OKAY), 0); // send the message to client just to refresh the connection
                        }
                        if (strstr(buffer, "LIST") != NULL) // if the command is LIST
                        {
                            list_function(); // call the list_function to list the directories of current directory
                            int sent = send(newSocket, list_string, strlen(list_string), 0); // send the message to client
                            memset(list_string, 0, sizeof(list_string)); // reset the list_string variable
                        }
                    }
                    else // if user is not logged in or typing the USER command first time,
                    {
                        char *message = "";
                        if (strstr(buffer, "USER") != NULL)
                        {
                            logged_in_array[newSocket] = 1; // set the variable to 1 i.e. mark the user as logged in
                            message = LOGIN_SUCCESS; // set the successful login message
                        }
                        else
                        {
                            message = LOGIN_REQUIRED; // set the message that login is required
                        }
                        int sent = send(newSocket, message, strlen(message), 0); // send the message to client
                    }
                }

                bzero(buffer, sizeof(buffer)); // reset the buffer 
            }
        }
    }

    close(newSocket); // close the socket

    return 0;
}