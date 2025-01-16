#include "functions.h"

int read_file(st_request *req)
{   
    printf("Reading file: %s\n", req->src_path);
    printf("Reading file data: %s\n", req->data);
    char buffer[CHUNK_SIZE];
    int file_fd;
    ssize_t bytes_read, bytes_sent;

    // Open the file for reading
    char full_path[MAX_PATH_LEN];
    memset(full_path, 0, sizeof(full_path));
    // strcpy(full_path, "main/");
    if(strcmp(req->data, "Backup | 1")==0){
        strcpy(full_path, "b1/");
    }
    else if(strcmp(req->data, "Backup | 2")==0){
        strcpy(full_path, "b2/");
    }
    else{
        strcpy(full_path, "main/");
    }
    printf("Full path: %s\n", full_path);
    strcat(full_path, req->src_path);
    printf("Full path: %s\n", full_path);
    file_fd = open(full_path, O_RDONLY);
    if (file_fd < 0)
    {
        perror("Failed to open file");
        // const char *error_msg = "Error: Unable to open the requested file.\n";
        // send(req->socket, error_msg, strlen(error_msg), 0);
        st_request error;
        error.request_type=FILE_READ_ERROR;
        strcpy(error.data, "Error: Unable to open the requested file.\n");
        if(send(req->socket, &error, sizeof(st_request), 0) < 0){
            perror("Failed to send error response to client");
        }
        else{
            printf("Error response sent to client\n");
        }
        return -1;
    }
    st_request read_request;
    // Read the file and send its content in chunks to the client
    while ((bytes_read = read(file_fd, read_request.data, CHUNK_SIZE)) > 0)
    {
        printf("Bytes read: %zd\n", bytes_read);
        printf("Data read: %s\n", read_request.data);

        bytes_sent = send(req->socket, &read_request, sizeof(st_request), 0);
        if (bytes_sent < 0)
        {
            perror("Failed to send data to client");
            close(file_fd);
            st_request error;
            error.request_type=FILE_READ_ERROR;
            strcpy(error.data, "Error: Unable to send the requested file.\n");
            if(send(req->socket, &error, sizeof(st_request), 0) < 0){
                perror("Failed to send error response to client");
            }
            else{
                printf("Error response sent to client\n");
            }
                return -1;
        }
        usleep(1000);
    }

    if (bytes_read < 0)
    {
        perror("Failed to read from file");
        st_request error;
        error.request_type=FILE_READ_ERROR;
        strcpy(error.data, "Error: Unable to send the requested file.\n");
        if(send(req->socket, &error, sizeof(st_request), 0) < 0){
            perror("Failed to send error response to client");
        }
        else{
            printf("Error response sent to client\n");
        }
        close(file_fd);
        return -1;
    }
    else
    {
        const char *success_msg = "File sent successfully.\n";
        printf("File sent successfully.\n");
        st_request ack;
        ack.request_type=ACK_MSG;
        strcpy(ack.data,success_msg);   
        send(req->socket, &ack, sizeof(st_request), 0);
    }

    // Close the file descriptor
    close(file_fd);
    return 0;
}