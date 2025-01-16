#include "headers.h"

#define BUFFER_SIZE 1024*100
void communicate_with_ss_stream(char *ipaddress, int port, char *path)
{   
    // printf("path at stream function:%s", path);1
    int client_socket = connect_with_ss(ipaddress, port);
    printf("Connected with storage server for streaming\n");
    st_request *readerpacket = malloc(sizeof(st_request));
    readerpacket->request_type = STREAM_AUDIO;
    strcpy(readerpacket->src_path, path);
    int bytes_sent = send(client_socket, readerpacket, sizeof(st_request), 0);
    if (bytes_sent == -1)
    {
        perror("Send failed");
    }
    int pipe_fds[2];

    // Create a pipe for sending data to mpg123
    if (pipe(pipe_fds) < 0)
    {
        perror("Pipe creation failed");
        close(client_socket);
        return;
    }

    // Fork a process to handle audio playback
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork failed");
        close(client_socket);
        return;
    }

    if (pid == 0)
    {
        // Child process: play audio using mpg123
        close(pipe_fds[1]);              // Close the write end of the pipe
        dup2(pipe_fds[0], STDIN_FILENO); // Redirect pipe read end to stdin
        close(pipe_fds[0]);              // Close the read end after redirection

        // Execute mpg123 to play audio from stdin
        execlp("mpg123", "mpg123", "-", NULL);
        perror("Failed to execute mpg123");
        exit(1);
    }
    else
    {
        // Parent process: receive audio data and send it to the pipe
        close(pipe_fds[0]); // Close the read end of the pipe
        char buffer[BUFFER_SIZE];
        int bytes_received;

        printf("Streaming audio in real-time...\n");

        while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0)
        {
            if (write(pipe_fds[1], buffer, bytes_received) != bytes_received)
            {
                perror("Failed to write to pipe");
                break;
            }
        }

        if (bytes_received < 0)
        {
            perror("Error receiving audio data");
        }

        close(pipe_fds[1]);   // Close the write end of the pipe
        close(client_socket); // Close the server socket
        printf("Audio streaming finished.\n");
    }
}
    
void stream(char *path)
{
    printf("%s\n",path);
    int client_socket = connect_with_ns();
    st_request readerpacket;
    readerpacket.request_type = STREAM_AUDIO;
    // char * path;
    // strcpy(path,argv[1]);
    strcpy(readerpacket.src_path, path);

    ssize_t bytes_sent = send(client_socket, &readerpacket, sizeof(st_request), 0);
    if (bytes_sent == -1)
    {
        perror("Send failed");
    }
    st_response response;
    ssize_t bytes_received = recv(client_socket, &response, sizeof(st_request), 0);
    int port;
    char *ipaddress = (char *)malloc(16 * sizeof(char));
    if (bytes_received == -1)
    {
        perror("Receive failed");
    }
    else
    {
        if (response.response_type == FILE_NOT_FOUND)
        {
            printf(RED_COLOR"File Not Found \n"RESET_COLOR); // Error Not Found File
            // return;
        }
        // else if (response.response_type == TIMEOUT)
        // {
        //     printf(RED("File currently unavailable please try again\n"));
        //     // return;
        // }
        else if (response.response_type == INVALID_FILETYPE)
        {
            printf(RED_COLOR"Invalid file type\n"RESET_COLOR);
            // return;
        }
        // else if (response->response_type == SERVER_NOT_FOUND)
        // {
        //     printf(RED("%s\n"), response->message);
        //     // close(client_socket);
        // }
        else
        {
            strcpy(ipaddress, response.IP_Addr);
            port = response.Port_No;
        }
    }
    close(client_socket);
    printf("%s\n",path);
    if (response.response_type == 200)
    {
        communicate_with_ss_stream(ipaddress, port, path);
    }
    // else if (response->response_type == BACKUP_READ_REQ)
    // {
    //     communicate_with_ss_backup(ipaddress, port, path);
    // }
}
