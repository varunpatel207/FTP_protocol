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

// handler function to handle CTRL-C and CTRL-Z signals
void kill_server_sig_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTSTP) // if the incoming signal is CTR-C (SIGNINT) or CTRL-Z (SIGTSTP) then
    {
        send(client_socket, EXIT, strlen(EXIT), 0); // send the exit message to server
        close(client_socket);                       // close the main socket
        printf("%s\n", SOCKET_CLOSED);              // print the socket closed message
        exit(1);                                    // exit from the program
    }
}

// function to split the command entered by user from whitespace (" ")
char *split_command(char *buffer)
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

// function to send file to server over data transfer socket
void send_file(char *filename, int sockfd)
{
    char data[1024]; // defining the array of 1024 bytes to store the data of files
    FILE *fp;
    fp = fopen(filename, "r");            // opening the file in read mode
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
}

// function to  write file sent from server over data transfer socket
int write_file(int sockfd, char *buffer)
{
    int n;
    FILE *fp;
    char *filename = split_command(buffer); // get the filename by splitting the buffer
    char data[1024] = "\0";                 // defining the variable to store the data coming from server and initializing with 0's

    fp = fopen(filename, "w"); // open the file in writting mode
    while (1)                  // infinite loop
    {
        n = recv(sockfd, data, 1024, 0); // receiving the data from server into the blocks of 1024 bytes in data variable
        if (n <= 0)                      // if nothing is getting returned from server then
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

int main()
{
    signal(SIGINT, kill_server_sig_handler);  // installing the handler for SIGINT (CTRL-C) signal
    signal(SIGTSTP, kill_server_sig_handler); // installing the handler for SIGTSTP (CTRL-Z) signal

    int ret;
    struct sockaddr_in server_addr;
    char buffer[1024];
    char response_buffer[1024];

    client_socket = socket(AF_INET, SOCK_STREAM, 0); // creating main socket to establish a connection
    if (client_socket < 0)                           // if socket is not created then
    {
        printf("%s\n", "Socket connection error"); // print this message
        exit(1);                                   // exit from the program
    }
    printf("%s\n", "Socket connected successfully");

    // socket connect for file transfer
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    // connect client socket to server socket
    ret = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)); // connect the socket to server's address
    if (ret < 0)                                                                        // if connection is not successful then
    {
        printf("%s\n", CONNECTION_ERROR); // print this message
        exit(1);                          // exit from the program
    }
    printf("%s\n", OPEN_CONNECTION_NO_TRANSFER);

    while (1)
    {
        int new_ret = -1;
        int new_client_socket;
        memset(buffer, 0, 1024);
        memset(response_buffer, 0, 1024);
        printf("Client: ");
        fgets(buffer, 1024, stdin); // get the command from stdin which user entered
        buffer[strcspn(buffer, "\n")] = 0;

        // code to handle STOR command
        // send file data to server on data transfer port
        if (strstr(buffer, "STOR") != NULL) // if user entered STOR command then,
        {
            fflush(stdout);
            char temp_buffer[1024];
            strcpy(temp_buffer, buffer); // copy the data of buffer into temp_buffer variable
            char file_real_path[1024];

            char *filename = split_command(temp_buffer); // get the filename by splitting the buffer

            realpath(filename, file_real_path); // get the realpath of filename

            send_file(file_real_path, new_client_socket); // use the send_file method to send the file to server using new socket which was created for file/data transfer
            close(new_client_socket);                     // close the socket
        }

        // code to handle APPE command
        // send file data to server on data transfer port
        if (strstr(buffer, "APPE") != NULL) // if user entered APPE command then,
        {
            fflush(stdout);
            char temp_buffer[1024];
            char file_real_path[1024];

            strcpy(temp_buffer, buffer); // copy the data of buffer into temp_buffer variable

            char *filename = split_command(temp_buffer); // get the filename by splitting the buffer

            realpath(filename, file_real_path); // get the realpath of filename

            send_file(file_real_path, new_client_socket); // use the send_file method to send the file to server using new socket which was created for file/data transfer
            close(new_client_socket);                     // close the socket
        }

        // send command to client
        send(client_socket, buffer, strlen(buffer), 0); // send the command to server

        // close client socket on exit
        if (strcmp(buffer, "QUIT") == 0) // if user entered quit command,
        {
            close(client_socket); // close the main socket
            printf("Disconnected from server.\n");
            exit(1); // exit from the program
        }

        int n_read = 0;
        if ((n_read = recv(client_socket, response_buffer, 1024, 0)) < 0) // receive the response from server
        {
            printf("Error in receiving data.\n");
        }
        else
        {
            // move to parent directory of CWD
            if (strcmp(response_buffer, "CDUP") == 0) // if the response buffer has CDUP then
            {
                char wd[100];
                printf("previous working directory : %s\n", getcwd(wd, 100)); // print the current working directory using getcwd
                char resolved_path[1024];
                char *parent_dir = dirname(wd);      // get the parent directory using dirname function
                chdir(parent_dir);                   // change the current working directory to parent directory
                printf("parent : %s\n", parent_dir); // print the directory name
            }

            // CWD on client side
            else if (strstr(response_buffer, "CWD") != NULL) // if the response buffer has CDUP then
            {
                char wd[100];
                char *dirname = split_command(buffer); // get the argument of CWD command

                // printf("%s\n", dirname);
                printf("previous working dir : %s\n", getcwd(wd, 100)); // print the current working directory using getcwd
                char dir_real_path[1024];
                realpath(dirname, dir_real_path);                            // get the realpath of directory
                chdir(dir_real_path);                                        // change the current working directory to the mentioned directory
                printf("current working directory : %s\n", getcwd(wd, 100)); // print the current working directory name
            }

            // handle PORT command reponse
            else if (strcmp(response_buffer, "Socket success.") == 0) // if the socket is successfully created for file transfer then,
            {
                char wd[100];
                char *port_number = split_command(buffer);                                                  // get the port number from command by splitting it
                int new_port = atoi(port_number);                                                           // convert it into number
                new_client_socket = socket(AF_INET, SOCK_STREAM, 0);                                        // create the new socket to communicate with client
                server_addr.sin_port = htons(new_port);                                                     // using the port number mentioned by user
                new_ret = connect(new_client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)); // connecting to server
                printf("%s\n", OPEN_CONNECTION_NO_TRANSFER);
            }

            // STOR command response
            else if (strstr(buffer, "STOR") != NULL) // if the user has entered STOR then
            {
                printf("Server: %s\n", response_buffer);
                printf("File uploaded successfully.\n"); // print that file uploaded successfully
                close(new_client_socket);                // close the socket which was created for file transfer
            }

            // APPE command response
            else if (strstr(buffer, "APPE") != NULL) // if the user has entered APPE then
            {
                printf("Server: %s\n", response_buffer);
                printf("File appended successfully.\n");
                close(new_client_socket); // close the socket which was created for file transfer
            }

            // RETR command response
            else if (strstr(buffer, "RETR") != NULL) // if the user has entered RETR then
            {
                fflush(stdout);
                char wd[100];
                char temp_buffer[1024];
                strcpy(temp_buffer, buffer);

                char *prev_file_name = split_command(temp_buffer); // get the filename from command by splitting it
                char file_name[1024];
                realpath(prev_file_name, file_name); // get the realpath of the file

                write_file(new_client_socket, buffer); // write into the file using newlh created socket

                close(new_client_socket); // close the socket
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