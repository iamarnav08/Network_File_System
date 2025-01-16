#include "functions.h"

int append_file(st_request* req) {
    int file_fd;
    ssize_t bytes_written;
    size_t data_len = strlen(req->data);

    // Open the file for writing (create if not exists, truncate if exists)
    char full_path[2*MAX_PATH_LEN];
    memset(full_path, 0, sizeof(full_path));
    strcpy(full_path, "/main/");
    strcat(full_path, req->src_path);
    file_fd = open(full_path, O_WRONLY | O_APPEND, 0644);
    if (file_fd < 0) {  
        perror("Failed to open file for writing");
        char *error_msg = "Error: Unable to open the file for writing.\n";
        st_response response;
        strcpy(response.message,error_msg);
        send(req->socket, &response, sizeof(st_response), 0);
        return -1;
    }

    printf("Writing to file: %s\n", req->src_path);

    // Write the data from st_request to the file
    bytes_written = write(file_fd, req->data, data_len);
    if (bytes_written < 0) {
        perror("Failed to write to file");
        close(file_fd);
        return -1;
    }
    // Ensure all the data has been written
    if ((size_t)bytes_written < data_len) {
        printf("Warning: Only %zd out of %zu bytes were written.\n", bytes_written, data_len);
    } else {
        printf("File written successfully: %s\n", req->src_path);
    }

    // Close the file descriptor
    close(file_fd);

    // Send acknowledgment to the client
    // st_request ack;
    // ack.request_type=ACK_MSG;
    // // const char *ack_msg = "ACK";
    // if (send(req->socket, &ack, sizeof(st_request), 0) < 0) {
    //     perror("Failed to send ACK to client");
    // } else {
    //     printf("ACK sent to client.\n");
    // }
    return 0;
}