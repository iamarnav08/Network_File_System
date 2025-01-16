#include "headers.h"

// Function to get the name of the requests
const char* get_request_name(int value) {
    switch (value) {
        case READ_FILE: return "READ_FILE";
        case WRITE_FILE: return "WRITE_FILE";
        case GET_FILE_INFO: return "GET_FILE_INFO";
        case STREAM_AUDIO: return "STREAM_AUDIO";
        case COPY_FILE: return "COPY_FILE";
        case DELETE_FILE: return "DELETE_FILE";
        case CREATE_FILE: return "CREATE_FILE";
        case COPY_DIR: return "COPY_DIR";
        case DELETE_DIR: return "DELETE_DIR";
        case CREATE_DIR: return "CREATE_DIR";
        case FILE_FOUND: return "FILE_FOUND";
        case STORAGE_SERVER_CONNECTION: return "STORAGE_SERVER_CONNECTION";
        case LIST_PATHS: return "LIST_PATHS";
        case RECIEVE_FILE: return "RECIEVE_FILE";
        case PING: return "PING";
        case ACK: return "ACK";
        case RECIEVE_DIR: return "RECIEVE_DIR";
        case INACTIVE_STORAGE_SERVER_ACTIVATED: return "INACTIVE_STORAGE_SERVER_ACTIVATED";
        case COPY_TO_SAME_FILE: return "COPY_TO_SAME_FILE";
        case COPY_TO_SAME_DIR: return "COPY_TO_SAME_DIR";
        case BACKUP_RECEIVE: return "BACKUP_RECEIVE";
        case BACKUP_SEND: return "BACKUP_SEND";
        default: return "INVALID";
    }
}

// Function to the name of the responses 
const char* get_respone_name(int value) {
    switch (value) {
        case INVALID_REQUEST: return "INVALID_REQUEST";
        case INVALID_FILETYPE: return "INVALID_FILETYPE";
        case FILE_NOT_FOUND: return "FILE_NOT_FOUND";
        case FILE_ALREADY_EXISTS: return "FILE_ALREADY_EXISTS";
        case FILE_WRITE_ERROR: return "FILE_WRITE_ERROR";
        case FILE_READ_ERROR: return "FILE_READ_ERROR";
        case FILE_DELETE_ERROR: return "FILE_DELETE_ERROR";
        case FILE_CREATE_ERROR: return "FILE_CREATE_ERROR";
        case FILE_COPY_ERROR: return "FILE_COPY_ERROR";
        case FILE_FOUND: return "FILE_FOUND";
        case COPY_TO_PATH_INVALID: return "COPY_TO_PATH_INVALID";
        case PATH_NOT_FOUND: return "PATH_NOT_FOUND";
        case PATH_FOUND: return "PATH_FOUND";
        default: return "INVALID";
    }
}

// Function to log messages to file
void logMessage(bool isClient, int client_socket, st_request request, int response_code, bool fromcache) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char IP_Addr[INET_ADDRSTRLEN];  // Buffer for storing the IP address
    int Port_No;

    // Get the client's address and port number
    if (getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len) == 0) {
        // Convert IP address to human-readable form
        if (inet_ntop(AF_INET, &client_addr.sin_addr, IP_Addr, sizeof(IP_Addr)) != NULL) {
            Port_No = ntohs(client_addr.sin_port);
        } else {
            perror("inet_ntop failed");
        }
    } else {
        perror("getpeername failed");
        return;
    }



    FILE* log_file = fopen("log.txt", "a");
    if (log_file == NULL) {
        printf("Error opening log file\n");
        return;
    }
    time_t current_time;
    time(&current_time);
    struct tm* time_info = localtime(&current_time);
    char time_str[30];
    strftime(time_str, 30, "%Y-%m-%d %H:%M:%S", time_info);
    if(isClient) {
        fprintf(log_file, "Client :\n");
    } else {
        fprintf(log_file, "Storage Server :\n");  
    }
    fprintf(log_file, "Time Stamp: %s\n", time_str);
    fprintf(log_file, "IP Address: %s\n", IP_Addr);
    fprintf(log_file, "Port Number: %d\n", Port_No);
    fprintf(log_file, "Request Type: %s\n", get_request_name(request.request_type));
    fprintf(log_file, "Response Code: %s\n", get_respone_name(response_code));
    if(fromcache) {
        fprintf(log_file, "Cache Hit\n");
    } else {
        fprintf(log_file, "Cache Miss\n");
    }
    fprintf(log_file, "\n");
    fclose(log_file);
}