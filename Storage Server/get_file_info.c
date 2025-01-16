#include "functions.h"

void format_permissions(mode_t mode, char *perm_str) {
    // File type
    perm_str[0] = S_ISDIR(mode) ? 'd' : '-';
    
    // Owner permissions
    perm_str[1] = (mode & S_IRUSR) ? 'r' : '-';
    perm_str[2] = (mode & S_IWUSR) ? 'w' : '-';
    perm_str[3] = (mode & S_IXUSR) ? 'x' : '-';
    
    // Group permissions
    perm_str[4] = (mode & S_IRGRP) ? 'r' : '-';
    perm_str[5] = (mode & S_IWGRP) ? 'w' : '-';
    perm_str[6] = (mode & S_IXGRP) ? 'x' : '-';
    
    // Others permissions
    perm_str[7] = (mode & S_IROTH) ? 'r' : '-';
    perm_str[8] = (mode & S_IWOTH) ? 'w' : '-';
    perm_str[9] = (mode & S_IXOTH) ? 'x' : '-';
    perm_str[10] = '\0'; // Null-terminate the string
}

#include <dirent.h>  // For directory handling
#include"functions.h"

int get_file_info(st_request* req) {
    struct stat file_stat;
    char response[4096];  // Buffer to hold the response
    char perm_str[11];    // To store formatted permissions string
    memset(response, 0, sizeof(response));
    memset(perm_str, 0, sizeof(perm_str));
    char full_path[2 * MAX_PATH_LEN];
    memset(full_path, 0, sizeof(full_path));
    snprintf(full_path, sizeof(full_path), "main/%s", req->src_path);

    // Get file information using stat()
    if (stat(full_path, &file_stat) < 0) {
        perror("Failed to retrieve file information");
        st_response error;
        error.response_type = GET_FILE_INFO_ERROR;
        strcpy(error.message, "Error: Failed to retrieve file information\n");
        if (send(req->socket, &error, sizeof(st_response), 0) < 0) {
            perror("Failed to send error response to client");
        } else {
            printf("Error response sent to client\n");
        }
        return -1;
    }

    // Format file permissions
    format_permissions(file_stat.st_mode, perm_str);

    // If it's a directory, list its contents
    if (S_ISDIR(file_stat.st_mode)) {
        DIR *dir;
        struct dirent *entry;

        snprintf(response, sizeof(response),
                 "Directory: %s\nPermissions: %s\nContents:\n",
                 req->src_path, perm_str);

        dir = opendir(full_path);
        if (dir == NULL) {
            perror("Failed to open directory");
            st_response error;
            error.response_type = GET_FILE_INFO_ERROR;
            strcpy(error.message, "Error: Failed to open directory\n");
            if (send(req->socket, &error, sizeof(st_response), 0) < 0) {
                perror("Failed to send error response to client");
            } else {
                printf("Error response sent to client\n");
            }
            return -1;
        }

        // Read directory contents
        while ((entry = readdir(dir)) != NULL) {
            strcat(response, entry->d_name);
            strcat(response, "\n");
        }
        closedir(dir);

    } else if (S_ISREG(file_stat.st_mode)) {  // If it's a regular file, send file information and content
        snprintf(response, sizeof(response),
                 "File: %s\nSize: %ld bytes\nPermissions: %s\nContent:\n",
                 req->src_path, file_stat.st_size, perm_str);

        FILE *file = fopen(full_path, "r");
        if (file == NULL) {
            perror("Failed to open file");
            st_response error;
            error.response_type = GET_FILE_INFO_ERROR;
            strcpy(error.message, "Error: Failed to open file\n");
            if (send(req->socket, &error, sizeof(st_response), 0) < 0) {
                perror("Failed to send error response to client");
            } else {
                printf("Error response sent to client\n");
            }
            return -1;
        }

        char file_content[2048];
        size_t bytes_read;
        while ((bytes_read = fread(file_content, 1, sizeof(file_content) - 1, file)) > 0) {
            file_content[bytes_read] = '\0';  // Null-terminate
            strcat(response, file_content);
        }
        fclose(file);
    } else {
        snprintf(response, sizeof(response), "Unsupported file type: %s\n", req->src_path);
    }

    // Send the file information to the client
    st_request req1;
    req1.request_type = 15;
    strcpy(req1.data, response);
    if (send(req->socket, &req1, sizeof(st_request), 0) < 0) {
        perror("Failed to send file information to client");
        st_response error;
        error.response_type = GET_FILE_INFO_ERROR;
        strcpy(error.message, "Error: Failed to send file information to client\n");
        if (send(req->socket, &error, sizeof(st_response), 0) < 0) {
            perror("Failed to send error response to client");
        } else {
            printf("Error response sent to client\n");
        }
        return -1;
    }

    printf("File information sent to client: %s\n", response);

    // Send acknowledgment to the client
    const char *ack_msg = "ACK";
    st_request ack;
    strcpy(ack.data, ack_msg);
    ack.request_type = ACK_MSG;
    if (send(req->socket, &ack, sizeof(st_request), 0) < 0) {
        perror("Failed to send ACK to client");
    } else {
        printf("ACK sent to client.\n");
    }

    return 0;
}