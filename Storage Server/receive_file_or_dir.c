#include "functions.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int receive_file(st_request* req, int client_sock) {
    printf("Receiving file at path: %s\n", req->dest_path);
    printf("rec file req->src_file_dir_name: %s\n", req->src_file_dir_name);

    char buffer[1024];
    ssize_t bytes_received;
    char full_path[MAX_PATH_LEN];
    strcpy(full_path, req->dest_path);
    strcat(full_path, "/");
    strcat(full_path, req->src_file_dir_name);
    printf("receiver full path:%s\n", full_path);
    
    // Open destination file for writing
    int dest_fd = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        perror("Failed to open destination file");
        return -1;
    }

    // Receive and write file data
    while ((bytes_received = recv(client_sock, buffer, sizeof(buffer), MSG_WAITALL)) > 0) {
        if (write(dest_fd, buffer, bytes_received) < 0) {
            perror("Failed to write to file");
            close(dest_fd);
            return -1;
        }
    }

    if (bytes_received < 0) {
        perror("Error receiving file data");
        close(dest_fd);
        return -1;
    }

    // Send acknowledgment to nag server
    // st_response ack;
    // ack.response_type = ACK_MSG;
    // if (send(ns_sock, &ack, sizeof(ack), 0) < 0) {
    //     perror("Failed to send ACK to Naming Server");
    // }
    // else {
    //     printf("ACK sent to Naming Server\n");
    // }

    close(dest_fd);
    printf("File received successfully at: %s\n", req->dest_path);
    return 0;
}

int receive_directory_structure(int client_sock, const char* base_path) {
    while (1) {
        file_info info;
        ssize_t bytes_received = recv(client_sock, &info, sizeof(info), MSG_WAITALL);
        if (bytes_received <= 0) return -1;

        if (info.is_end) return 0;

        char entry_path[MAX_COPY_LEN];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", base_path, info.file_name);

        if (info.is_dir) {
            if (mkdir(entry_path, 0755) < 0 && errno != EEXIST) {
                perror("Failed to create subdirectory");
                return -1;
            }
            printf("Created directory: %s\n", entry_path);
            
            if (receive_directory_structure(client_sock, entry_path) < 0) {
                return -1;
            }
        }
    }
}

int receive_files(int client_sock, const char* base_path) {
    while (1) {
        file_info info;
        ssize_t bytes_received = recv(client_sock, &info, sizeof(info), MSG_WAITALL);
        if (bytes_received <= 0) return -1;

        if (info.is_end) return 0;

        char entry_path[MAX_COPY_LEN];
        printf("base_path: %s\n", base_path);
        printf("info.file_name: %s\n", info.file_name);
        snprintf(entry_path, sizeof(entry_path), "%s/%s", base_path, info.file_name);
        printf("entry_path: %s\n", entry_path);
        // snprintf(entry_path, sizeof(entry_path), "%s", info.file_name);

        if (!info.is_dir) {
            printf("creating file at: %s\n", entry_path);
            int dest_fd = open(entry_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (dest_fd < 0) {
                perror("Failed to create file");
                return -1;
            }

            size_t remaining = info.file_size;
            char buffer[1024];
            while (remaining > 0) {
                bytes_received = recv(client_sock, buffer, 
                                   MIN(sizeof(buffer), remaining), MSG_WAITALL);
                if (bytes_received <= 0) {
                    close(dest_fd);
                    return -1;
                }
                if (write(dest_fd, buffer, bytes_received) < 0) {
                    close(dest_fd);
                    return -1;
                }
                remaining -= bytes_received;
            }
            close(dest_fd);
            printf("Received file: %s\n", entry_path);
        } else {
            if (receive_files(client_sock, entry_path) < 0) {
                return -1;
            }
        }
    }
}


int receive_directory(st_request* req, int client_sock) {
    printf("Receiving directory at: %s\n", req->dest_path);
    
    char full_path[MAX_PATH_LEN];
    char backup_folder[2];
    strcpy(backup_folder, req->dest_path);
    printf("backup_folder: %s\n", backup_folder);   
    if (strcmp(req->src_file_dir_name, "b1")==0 || 
        strcmp(req->src_file_dir_name, "b2")==0 || 
        strcmp(req->src_file_dir_name, "b3")==0) {
        
        if (strlen(req->src_file_dir_name) >= MAX_PATH_LEN) {
            fprintf(stderr, "Backup directory name too long\n");
            return -1;
        }
        strncpy(full_path, req->src_file_dir_name, MAX_PATH_LEN - 1);
        full_path[MAX_PATH_LEN - 1] = '\0';
    } else {
        size_t required_len = strlen(req->dest_path) + 1 + strlen(req->src_file_dir_name) + 1;
        if (required_len > MAX_PATH_LEN) {
            fprintf(stderr, "Combined path too long\n");
            return -1;
        }
        
        if (snprintf(full_path, MAX_PATH_LEN, "%s/%s", 
                    req->dest_path, req->src_file_dir_name) >= MAX_PATH_LEN) {
            fprintf(stderr, "Path truncated\n");
            return -1;
        }
    }
    printf("trying to create directory at: %s\n", full_path);
    if (mkdir(full_path, 0755) < 0 && errno != EEXIST) {
        perror("Failed to create destination directory");
        return -1;
    }

    // First pass: Receive directory structure
    if (receive_directory_structure(client_sock, full_path) < 0) {
        return -1;
    }
    // printf("Directory structure received successfully at: %s\n", full_path);
    // strncpy(full_path, req->src_file_dir_name, MAX_PATH_LEN - 1);
    // printf("copying files at: %s\n", full_path);
    // Second pass: Receive files
    if (receive_files(client_sock, req->dest_path) < 0) {
        return -1;
    }

    printf("Directory received successfully at: %s\n", full_path);
    return 0;
}