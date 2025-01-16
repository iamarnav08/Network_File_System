#include "functions.h"

int copy_file(st_request* req) {
    printf("at copy file Copying file: %s\n", req->src_path);
    int src_fd;
    char buffer[1024];
    ssize_t bytes_read, bytes_sent;

    // Connect to destination server
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(req->port_for_copy);
    if (inet_pton(AF_INET, req->ip_for_copy, &dest_addr.sin_addr) < 0) {
        perror("Invalid destination IP address");
        return -1;
    }
    printf("ip for copy-%s\n", req->ip_for_copy);
    printf("port for copy-%d\n", req->port_for_copy);

    // Create new socket for connection
    int copy_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (copy_sock < 0) {
        perror("Failed to create socket");
        return -1;
    }

    if (connect(copy_sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Failed to connect to destination server");
        close(copy_sock);
        return -1;
    }

    // Open source file
    src_fd = open(req->src_path, O_RDONLY);
    if (src_fd < 0) {
        perror("Failed to open source file");
        st_response error;
        error.response_type = FILE_COPY_ERROR;
        strcpy(error.message, "Error: Unable to open the source file for copying\n");
        send(req->socket, &error, sizeof(st_response), 0);
        close(copy_sock);
        return -1;
    }

    // Send file data using the same socket used for connection
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_sent = send(copy_sock, buffer, bytes_read, 0);
        if (bytes_sent < 0) {
            perror("Failed to send file data");
            close(src_fd);
            close(copy_sock);
            return -1;
        }
    }

    close(src_fd);
    close(copy_sock);
    printf("File copy complete: %s\n", req->src_path);
    return 0;
}


int create_directory_structure(int copy_sock, const char* current_path) {
    DIR* dir = opendir(current_path);
    if (!dir) {
        perror("Failed to open source directory");
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_entry_path[MAX_COPY_LEN];
        snprintf(src_entry_path, sizeof(src_entry_path), "%s/%s", current_path, entry->d_name);

        struct stat stat_buf;
        if (stat(src_entry_path, &stat_buf) < 0) {
            continue;
        }

        if (S_ISDIR(stat_buf.st_mode)) {
            file_info info;
            strncpy(info.file_name, entry->d_name, MAX_FILE_NAME - 1);
            info.is_dir = 1;
            info.file_size = 0;
            info.is_end = 0;

            if (send(copy_sock, &info, sizeof(info), MSG_NOSIGNAL) < 0) {
                perror("Failed to send directory info");
                closedir(dir);
                return -1;
            }

            if (create_directory_structure(copy_sock, src_entry_path) < 0) {
                closedir(dir);
                return -1;
            }
        }
    }

    // Send end marker
    file_info end_marker = {0};
    end_marker.is_end = 1;
    if (send(copy_sock, &end_marker, sizeof(end_marker), MSG_NOSIGNAL) < 0) {
        closedir(dir);
        return -1;
    }

    closedir(dir);
    return 0;
}

int copy_files(int copy_sock, const char* current_path) {
    DIR* dir = opendir(current_path);
    if (!dir) {
        perror("Failed to open source directory");
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_entry_path[MAX_COPY_LEN];
        printf("current path: %s\n", current_path);
        printf("entry name: %s\n", entry->d_name);
        snprintf(src_entry_path, sizeof(src_entry_path), "%s/%s", current_path, entry->d_name);
        printf("copying file from home directory %s to %s\n", current_path, src_entry_path);

        struct stat stat_buf;
        if (stat(src_entry_path, &stat_buf) < 0) {
            continue;
        }

        if (S_ISDIR(stat_buf.st_mode)) {
            if (copy_files(copy_sock, src_entry_path) < 0) {
                closedir(dir);
                return -1;
            }
        } else {
            file_info info;
            strncpy(info.file_name, src_entry_path, MAX_FILE_NAME - 1);
            printf("file name: %s\n", info.file_name);
           if (strncmp(info.file_name, "main/", 5) == 0) {
                size_t remaining_len = strlen(info.file_name) - 5 + 1; // Total length minus prefix plus null
                memmove(info.file_name, info.file_name + 5, remaining_len);
                info.file_name[remaining_len - 1] = '\0'; // Ensure null termination
                printf("new file name: %s\n", info.file_name); // Debug print should now work
            }
            printf("new file name: %s\n", info.file_name); // Debug print should now work
            info.is_dir = 0;
            info.file_size = stat_buf.st_size;
            info.is_end = 0;

            if (send(copy_sock, &info, sizeof(info), MSG_NOSIGNAL) < 0) {
                closedir(dir);
                return -1;
            }

            int src_fd = open(src_entry_path, O_RDONLY);
            if (src_fd < 0) {
                closedir(dir);
                return -1;
            }

            char buffer[1024];
            ssize_t bytes_read;
            while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
                if (send(copy_sock, buffer, bytes_read, MSG_NOSIGNAL) < 0) {
                    close(src_fd);
                    closedir(dir);
                    return -1;
                }
            }
            close(src_fd);
        }
    }

    // Send end marker
    file_info end_marker = {0};
    end_marker.is_end = 1;
    if (send(copy_sock, &end_marker, sizeof(end_marker), MSG_NOSIGNAL) < 0) {
        closedir(dir);
        return -1;
    }

    closedir(dir);
    return 0;
}

int copy_directory(st_request* req) {
    printf("Copying directory from: %s\n", req->src_path);
    
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(req->port_for_copy);
    if (inet_pton(AF_INET, req->ip_for_copy, &dest_addr.sin_addr) < 0) {
        perror("Invalid destination IP address");
        return -1;
    }

    int copy_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (copy_sock < 0) {
        perror("Failed to create socket");
        return -1;
    }

    if (connect(copy_sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Failed to connect");
        close(copy_sock);
        return -1;
    }

    // First pass: Create directory structure
    if (create_directory_structure(copy_sock, req->src_path) < 0) {
        close(copy_sock);
        return -1;
    }

    // Second pass: Copy files
    if (copy_files(copy_sock, req->src_path) < 0) {
        close(copy_sock);
        return -1;
    }

    close(copy_sock);
    return 0;
}