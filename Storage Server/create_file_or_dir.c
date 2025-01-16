#include "functions.h"

int create_file_or_directory(st_request* req) {
    char full_path[2*MAX_PATH_LEN];
    memset(full_path, 0, sizeof(full_path));
    printf("src path at create_file_or_directory: %s\n", req->src_path);

    // Construct the full path
    // snprintf(full_path, sizeof(full_path), "main/%s/%s", req->src_path, req->src_file_dir_name);
    // Construct the full path
    // if (strcmp(req->src_path, "/") == 0) {
    //     // snprintf(full_path, sizeof(full_path), "main/%s", req->src_file_dir_name);
    //     strcpy(full_path, "main/");
    //     strcat(full_path, req->src_file_dir_name);
    // } else {
    //     // snprintf(full_path, sizeof(full_path), "main/%s/%s", req->src_path, req->src_file_dir_name);
    //     strcpy(full_path, "main/");
    //     strcat(full_path, req->src_path);
    //     strcat(full_path, "/");
    //     strcat(full_path, req->src_file_dir_name);
    // }
    strcpy(full_path, req->src_path);
    strcat(full_path, "/");
    strcat(full_path, req->src_file_dir_name);

    printf("creating fil/dir full path: %s\n", full_path);
    if (req->request_type ==CREATE_FILE) { // Create file
        int fd = open(full_path, O_CREAT | O_WRONLY, 0644); // rw-r--r-- permissions
        if (fd < 0) {
            perror("Failed to create file");
            // const char *error_msg = "Error: Unable to create file\n";
            // send(req->socket, error_msg, strlen(error_msg), 0);
            // return -1;
            st_response error;
            error.response_type=FILE_CREATE_ERROR;
            if(send(req->socket, &error, sizeof(st_response), 0) < 0){
                perror("Failed to send error response to client");
            }
            else{
                printf("Error response sent to client\n");
            }
        } else {
            printf("File created: %s\n", full_path);
            close(fd);

            const char *success_msg = "File created successfully\n";
            send(req->socket, success_msg, strlen(success_msg), 0);
        }
    } else if (req->request_type == CREATE_DIR) { // Create directory
        if (mkdir(full_path, 0755) < 0) { // rwxr-xr-x permissions
            perror("Failed to create directory");
            st_response error;
            error.response_type=FILE_CREATE_ERROR;
            if(send(req->socket, &error, sizeof(st_response), 0) < 0){
                perror("Failed to send error response to client");
            }
            else{
                printf("Error response sent to client\n");
            }
            return -1;
        } else {
            printf("Directory created: %s\n", full_path);
            const char *success_msg = "Directory created successfully\n";
            send(req->socket, success_msg, strlen(success_msg), 0);
        }
    } else {
        st_response error;
            error.response_type=INVALID_REQUEST;
            if(send(req->socket, &error, sizeof(st_response), 0) < 0){
                perror("Failed to send error response to client");
            }
            else{
                printf("Error response sent to client\n");
            }
    }
    add_path_lock(&lock_table, full_path);


    // Send acknowledgment
    // const char *ack_msg = "ACK";
    // if (send(req->socket, ack_msg, strlen(ack_msg), 0) < 0) {
    //     perror("Failed to send ACK to client");
    // } else {
    //     printf("ACK sent to client.\n");
    // }
    // st_response ack;
    // ack.response_type=ACK_MSG;
    // if(send(req->socket, &ack, sizeof(st_response), 0) < 0){
    //     perror("Failed to send ACK to client");
    // }
    // else{
    //     printf("ACK sent to client\n");
    // }
    return 0;
}