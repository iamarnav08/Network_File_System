#include"headers.h"

// Command types for the protocol
typedef enum {
    CMD_CREATE_DIR = 1,
    CMD_SEND_FILE = 2
} CommandType;

// Header structure for file transfers
typedef struct {
    CommandType cmd;
    char rel_path[MAX_COPY_LEN];  // Relative path from base directory
    size_t size;                  // File size (for CMD_SEND_FILE)
} TransferHeader;

// Function prototypes
int send_to_server(const char *ss2_ip, int ss2_port, CommandType cmd, const char *rel_path, const char *full_path);
int process_directory(const char *base_path, const char *curr_path,  const char *ss2_ip, int ss2_port);
int send_file(int sock_fd, const char *filepath);

// Helper function to create relative path
char* get_relative_path(const char *base_path, const char *full_path) {
    static char rel_path[MAX_PATH_LEN];
    size_t base_len = strlen(base_path);
    
    // Skip base path and any leading slash
    const char *start = full_path + base_len;
    while (*start == '/') start++;
    
    strncpy(rel_path, start, sizeof(rel_path) - 1);
    rel_path[sizeof(rel_path) - 1] = '\0';
    return rel_path;
}

// Send header and get acknowledgment
int send_header(int sock_fd, CommandType cmd, const char *rel_path, size_t size) {
    TransferHeader header = {
        .cmd = cmd,
        .size = size
    };
    strncpy(header.rel_path, rel_path, sizeof(header.rel_path) - 1);

    if (send(sock_fd, &header, sizeof(header), 0) != sizeof(header)) {
        perror("Failed to send header");
        return -1;
    }

    // Wait for acknowledgment
    char ack;
    if (recv(sock_fd, &ack, 1, 0) != 1) {
        perror("Failed to receive acknowledgment");
        return -1;
    }
    return 0;
}

// Send a file to the server
int send_file(int sock_fd, const char *filepath) {
    char buffer[BUFFER_SIZE];
    int file_fd = open(filepath, O_RDONLY);
    if (file_fd < 0) {
        perror("Failed to open file");
        return -1;
    }

    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        ssize_t bytes_sent = 0;
        while (bytes_sent < bytes_read) {
            ssize_t result = send(sock_fd, buffer + bytes_sent, 
                                bytes_read - bytes_sent, 0);
            if (result < 0) {
                if (errno == EINTR) continue;
                perror("Failed to send file data");
                close(file_fd);
                return -1;
            }
            bytes_sent += result;
        }
    }

    close(file_fd);
    return bytes_read < 0 ? -1 : 0;
}

// Establish connection and send command to server
int send_to_server(const char *ss2_ip, int ss2_port, CommandType cmd, const char *rel_path, const char *full_path) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(ss2_port)
    };
    if (inet_pton(AF_INET, ss2_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock_fd);
        return -1;
    }

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock_fd);
        return -1;
    }

    size_t size = 0;
    if (cmd == CMD_SEND_FILE) {
        struct stat st;
        if (stat(full_path, &st) < 0) {
            perror("Failed to get file size");
            close(sock_fd);
            return -1;
        }
        size = st.st_size;
    }

    if (send_header(sock_fd, cmd, rel_path, size) < 0) {
        close(sock_fd);
        return -1;
    }

    int result = 0;
    if (cmd == CMD_SEND_FILE) {
        result = send_file(sock_fd, full_path);
    }

    close(sock_fd);
    return result;
}

// Process a directory recursively
int process_directory(const char *base_path, const char *curr_path, const char *ss2_ip, int ss2_port) {
    DIR *dir = opendir(curr_path);
    if (!dir) {
        perror("Failed to open directory");
        return -1;
    }

    // Send directory creation command
    char *rel_path = get_relative_path(base_path, curr_path);
    if (send_to_server(ss2_ip, ss2_port, CMD_CREATE_DIR, rel_path, curr_path) < 0) {
        closedir(dir);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char path[MAX_COPY_LEN];
        snprintf(path, sizeof(path), "%s/%s", curr_path, entry->d_name);

        struct stat path_stat;
        if (stat(path, &path_stat) < 0) {
            perror("Failed to get file status");
            continue;
        }

        if (S_ISDIR(path_stat.st_mode)) {
            if (process_directory(base_path, path, ss2_ip, ss2_port) < 0) {
                fprintf(stderr, "Failed to process directory: %s\n", path);
            }
        } else if (S_ISREG(path_stat.st_mode)) {
            rel_path = get_relative_path(base_path, path);
            if (send_to_server(ss2_ip, ss2_port, CMD_SEND_FILE, rel_path, path) < 0) {
                fprintf(stderr, "Failed to send file: %s\n", path);
            } else {
                printf("Sent file: %s\n", path);
            }
        }
    }

    closedir(dir);
    return 0;
}

int helper_send_backup(st_request* req) {

    char* path = (char*)malloc(sizeof(char)*MAX_PATH_LEN);
    char* ss2_ip = (char*)malloc(sizeof(char)*IP_ADDR_LEN);
    strcpy(path, req->src_path);
    strcpy(ss2_ip, req->ip_for_copy);
    int ss2_port = req->port_for_copy;

    struct stat path_stat;
    if (stat(path, &path_stat) < 0) {
        perror("Failed to get path status");
        exit(1);
    }

    if (S_ISDIR(path_stat.st_mode)) {
        return process_directory(path, path, ss2_ip, ss2_port);
    } else if (S_ISREG(path_stat.st_mode)) {
        char *rel_path = basename(path);
        return send_to_server(ss2_ip, ss2_port, CMD_SEND_FILE, rel_path, path);
    } else {
        fprintf(stderr, "Invalid path type\n");
        return 1;
    }
}

