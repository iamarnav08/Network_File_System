#include "functions.h"

int copy_local_file(st_request* req) {
    printf("Copying local file from: %s to: %s\n", req->src_path, req->dest_path);
    
    char src_full_path[MAX_COPY_LEN];
    char dest_full_path[MAX_COPY_LEN];
    
    // Construct full paths
    snprintf(src_full_path, sizeof(src_full_path), "%s/%s", req->src_path, req->src_file_dir_name);
    snprintf(dest_full_path, sizeof(dest_full_path), "%s/%s", req->dest_path, req->src_file_dir_name);
    
    // Open source file
    int src_fd = open(src_full_path, O_RDONLY);
    if (src_fd < 0) {
        perror("Failed to open source file");
        return -1;
    }
    
    // Create destination file
    int dest_fd = open(dest_full_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        perror("Failed to create destination file");
        close(src_fd);
        return -1;
    }
    
    // Copy data
    char buffer[1024];
    ssize_t bytes_read, bytes_written;
    
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written < 0) {
            perror("Failed to write to destination file");
            close(src_fd);
            close(dest_fd);
            return -1;
        }
    }
    
    close(src_fd);
    close(dest_fd);
    printf("Local file copy complete\n");
    return 0;
}

int copy_local_directory(st_request* req) {
    char src_full_path[MAX_COPY_LEN];
    char dest_full_path[MAX_COPY_LEN];
    
    // Construct full paths
    snprintf(src_full_path, sizeof(src_full_path), "%s/%s", req->src_path, req->src_file_dir_name);
    snprintf(dest_full_path, sizeof(dest_full_path), "%s/%s", req->dest_path, req->src_file_dir_name);
    
    // Create destination directory
    if (mkdir(dest_full_path, 0755) < 0 && errno != EEXIST) {
        perror("Failed to create destination directory");
        return -1;
    }
    
    DIR* dir = opendir(src_full_path);
    if (!dir) {
        perror("Failed to open source directory");
        return -1;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char src_entry_path[2*MAX_COPY_LEN];
        char dest_entry_path[2*MAX_COPY_LEN];
        snprintf(src_entry_path, sizeof(src_entry_path), "%s/%s", src_full_path, entry->d_name);
        snprintf(dest_entry_path, sizeof(dest_entry_path), "%s/%s", dest_full_path, entry->d_name);
        
        struct stat stat_buf;
        if (stat(src_entry_path, &stat_buf) < 0) {
            perror("Failed to get file status");
            continue;
        }
        
        if (S_ISDIR(stat_buf.st_mode)) {
            // Recursively copy subdirectory
            st_request subreq = *req;
            strcpy(subreq.src_path, src_full_path);
            strcpy(subreq.dest_path, dest_full_path);
            strcpy(subreq.src_file_dir_name, entry->d_name);
            if (copy_local_directory(&subreq) < 0) {
                closedir(dir);
                return -1;
            }
        } else {
            // Copy regular file
            st_request subreq = *req;
            strcpy(subreq.src_path, src_full_path);
            strcpy(subreq.dest_path, dest_full_path);
            strcpy(subreq.src_file_dir_name, entry->d_name);
            if (copy_local_file(&subreq) < 0) {
                closedir(dir);
                return -1;
            }
        }
    }
    
    closedir(dir);
    printf("Local directory copy complete\n");
    return 0;
}