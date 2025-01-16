#include "functions.h"

int delete_file_or_directory_recursive(const char* path) {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0) {
        perror("Failed to stat path");
        return -1;
    }

    if (S_ISREG(path_stat.st_mode)) {
        // Delete the file
        if (unlink(path) < 0) {
            perror("Failed to delete file");
            return -1;
        }
        printf("Deleted file: %s\n", path);
    } else if (S_ISDIR(path_stat.st_mode)) {
        // Open the directory
        DIR* dir = opendir(path);
        if (!dir) {
            perror("Failed to open directory");
            return -1;
        }

        struct dirent* entry;
        char entry_path[MAX_PATH_LEN];
        while ((entry = readdir(dir)) != NULL) {
            // Skip "." and ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // Construct full path for the entry
            // snprintf(entry_path, sizeof(entry_path), "%s/%s", path, entry->d_name);
            strcpy(entry_path, path);
            strcat(entry_path, "/");
            strcat(entry_path, entry->d_name);

            // Recursively delete entries
            if (delete_file_or_directory_recursive(entry_path) < 0) {
                closedir(dir);
                return -1;
            }
        }

        closedir(dir);

        // Remove the directory itself
        if (rmdir(path) < 0) {
            perror("Failed to remove directory");
            return -1;
        }
        printf("Deleted directory: %s\n", path);
    }

    return 0;
}

int delete_file_or_directory(st_request* req) {
    char full_path[2 * MAX_PATH_LEN];
    memset(full_path, 0, sizeof(full_path));

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
    printf("Full path: %s\n", full_path);

    // Check if the file/directory exists
    struct stat path_stat;
    if (stat(full_path, &path_stat) < 0) {
        perror("Stat failed");
        const char* error_msg = "Error: File or directory not found\n";
        send(req->socket, error_msg, strlen(error_msg), 0);
        return -1;
    }

    // Perform recursive deletion
    if (delete_file_or_directory_recursive(full_path) < 0) {
        const char* error_msg = "Error: Unable to delete file/directory\n";
        send(req->socket, error_msg, strlen(error_msg), 0);
        return -1;
    }

    const char* success_msg = "File/directory deleted successfully\n";
    send(req->socket, success_msg, strlen(success_msg), 0);

    // Send acknowledgment
    const char* ack_msg = "ACK";
    st_request ack;
    ack.request_type = ACK_MSG;
    strcpy(ack.data, ack_msg);
    if (send(req->socket, &ack, sizeof(st_request), 0) < 0) {
        perror("Failed to send ACK to client");
    } else {
        printf("ACK sent to client.\n");
    }

    return 0;
}
