#include "headers.h"
#include<poll.h>

// Command types for the protocol
typedef enum
{
    CMD_CREATE_DIR = 1,
    CMD_SEND_FILE = 2
} CommandType;

// Header structure for file transfers
typedef struct
{
    CommandType cmd;
    char rel_path[MAX_COPY_LEN]; // Relative path from base directory
    size_t size;                 // File size (for CMD_SEND_FILE)
} TransferHeader;

// Function prototypes
int create_directory(const char *dirpath);
int receive_file1(int client_sock, const char *filepath, size_t size);
int handle_client(int client_sock, const char *base_path);

// Create all directories in path
int create_directories(const char *path)
{
    char tmp[MAX_COPY_LEN];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
    {
        tmp[len - 1] = 0;
    }

    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            mkdir(tmp, 0777);
            *p = '/';
        }
    }
    return mkdir(tmp, 0777);
}

// Create a directory and its parent directories if needed
int create_directory(const char *dirpath)
{
    if (mkdir(dirpath, 0777) == 0 || errno == EEXIST)
    {
        printf("Directory ready: %s\n", dirpath);
        return 0;
    }

    if (errno == ENOENT)
    {
        // Parent directory doesn't exist, try to create it
        if (create_directories(dirpath) == 0 || errno == EEXIST)
        {
            return 0;
        }
    }

    perror("Failed to create directory");
    return -1;
}

// Receive a file from the client
int receive_file1(int client_sock, const char *filepath, size_t size)
{
    // Create parent directories if they don't exist
    char dir_path[MAX_COPY_LEN];
    strncpy(dir_path, filepath, sizeof(dir_path));
    char *parent = dirname(dir_path);
    if (create_directory(parent) < 0)
    {
        return -1;
    }

    int file_fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd < 0)
    {
        perror("Failed to open file for writing");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    size_t remaining = size;
    size_t total_received = 0;

    while (remaining > 0)
    {
        size_t to_read = remaining < BUFFER_SIZE ? remaining : BUFFER_SIZE;
        ssize_t bytes_received = recv(client_sock, buffer, to_read, 0);

        if (bytes_received <= 0)
        {
            if (errno == EINTR)
                continue;
            perror("Failed to receive file data");
            close(file_fd);
            return -1;
        }

        ssize_t bytes_written = 0;
        while (bytes_written < bytes_received)
        {
            ssize_t result = write(file_fd, buffer + bytes_written,
                                   bytes_received - bytes_written);
            if (result < 0)
            {
                if (errno == EINTR)
                    continue;
                perror("Failed to write to file");
                close(file_fd);
                return -1;
            }
            bytes_written += result;
        }

        remaining -= bytes_received;
        total_received += bytes_received;

        // Print progress for large files
        if (size > BUFFER_SIZE * 10)
        {
            printf("\rReceiving %s: %.1f%%", filepath,
                   (double)total_received * 100.0 / size);
            fflush(stdout);
        }
    }

    if (size > BUFFER_SIZE * 10)
    {
        printf("\n");
    }

    close(file_fd);
    printf("Received file: %s (%zu bytes)\n", filepath, size);
    return 0;
}

// Handle a client connection
int handle_client(int client_sock, const char *base_path)
{
    TransferHeader header;

    // Receive header
    ssize_t bytes_received = recv(client_sock, &header, sizeof(header), MSG_WAITALL);
    if (bytes_received != sizeof(header))
    {
        perror("Failed to receive header");
        return -1;
    }

    // Send acknowledgment
    char ack = 1;
    if (send(client_sock, &ack, 1, 0) != 1)
    {
        perror("Failed to send acknowledgment");
        return -1;
    }

    // Construct full path
    char full_path[2 * MAX_COPY_LEN];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, header.rel_path);

    // Handle command
    switch (header.cmd)
    {
    case CMD_CREATE_DIR:
        return create_directory(full_path);

    case CMD_SEND_FILE:
        return receive_file1(client_sock, full_path, header.size);

    default:
        fprintf(stderr, "Unknown command: %d\n", header.cmd);
        return -1;
    }
}

int helper_receive_backup(st_request* req, int client_port, int server_sock) {
    const char *listen_ip = req->ip_for_copy;
    int listen_port = req->port_for_copy;
    char base_path1[MAX_PATH_LEN];
    strcpy(base_path1, req->dest_path);

    printf("Server listening on %s:%d\n", listen_ip, listen_port);
    printf("Files will be stored in: %s\n", base_path1);

    // Create base directory if it doesn't exist
    if (create_directory(base_path1) < 0) {
        fprintf(stderr, "Failed to create base directory: %s\n", base_path1);
        exit(1);
    }

    printf("Server listening on %s:%d\n", listen_ip, listen_port);
    printf("Files will be stored in: %s\n", base_path1);

    struct pollfd fds[1];
    fds[0].fd = server_sock;
    fds[0].events = POLLIN;

    while (1) {
        int ret = poll(fds, 1, 20); // 20 milliseconds timeout
        if (ret == -1) {
            perror("poll failed");
            break;
        } else if (ret == 0) {
            // Timeout occurred, exit the loop
            printf("Timeout occurred, exiting the loop\n");
            break;
        } else {
            if (fds[0].revents & POLLIN) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
                if (client_sock < 0) {
                    perror("accept failed");
                    continue;
                }

                // Handle the received request
                TransferHeader header;
                ssize_t bytes_received = recv(client_sock, &header, sizeof(header), MSG_WAITALL);
                if (bytes_received != sizeof(header)) {
                    perror("Failed to receive header");
                    close(client_sock);
                    continue;
                }

                // Send acknowledgment
                char ack = 1;
                if (send(client_sock, &ack, 1, 0) != 1) {
                    perror("Failed to send acknowledgment");
                    close(client_sock);
                    continue;
                }

                // Construct full path
                char full_path[2 * 4000];
                snprintf(full_path, sizeof(full_path), "%s/%s", base_path1, header.rel_path);

                // Handle command
                switch (header.cmd) {
                    case CMD_CREATE_DIR:
                        create_directory(full_path);
                        break;
                    case CMD_SEND_FILE:
                        receive_file1(client_sock, full_path, header.size);
                        break;
                    default:
                        fprintf(stderr, "Unknown command: %d\n", header.cmd);
                        break;
                }

                close(client_sock);
            }
        }
    }

    return 0;
}
