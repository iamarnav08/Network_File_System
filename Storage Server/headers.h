#ifndef _HEADERS_H_
#define _HEADERS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <netdb.h>
#include <libgen.h>
#include <semaphore.h>
#include <pthread.h>


#define BUFFER_SIZE 4096
#define MAX_COPY_LEN 4096
#define BACKLOG 10

// colors for printing
#define GREEN_COLOR  "\033[0;32m"
#define CYAN_COLOR   "\033[0;36m"
#define ORANGE_COLOR "\e[38;2;255;85;0m"
#define RESET_COLOR  "\033[0m"
#define BLUE_COLOR   "\033[0;34m"
#define RED_COLOR    "\033[0;31m"
#define YELLOW_COLOR "\033[0;33m"

// port defining
#define NS_PORT 3000
#define NS_IP "0.0.0.0"
#define NS_LISTEN_PORT 4096
#define CLIENT_LISTEN_PORT 4097
#define PING_PORT 1200

// max limits set
#define MAX_CLIENTS 100
#define MAX_THREADS 80
#define MAX_DATA_LENGTH 100000
#define MAX_CONNECTIONS 120
#define MAX_FILE_NAME 30
#define MAX_REQUEST_LEN 1000
#define MAX_FILES 25
#define MAX_PATHS 100
#define MAX_PATH_LEN 50
#define IP_ADDR_LEN 16
#define CHUNK_SIZE 1000

// flags
#define STORAGE_FLAG 0
#define CLIENT_FLAG 1

// Error codes
#define INVALID_REQUEST 400
#define INVALID_FILETYPE 402
#define FILE_NOT_FOUND 404
#define FILE_ALREADY_EXISTS 405
#define FILE_WRITE_ERROR 406
#define FILE_READ_ERROR 407
#define FILE_DELETE_ERROR 408
#define FILE_CREATE_ERROR 409
#define FILE_COPY_ERROR 410
#define COPY_TO_PATH_INVALID 411
#define PATH_NOT_FOUND 412
#define GET_FILE_INFO_ERROR 414

// Request types
#define READ_FILE 1
#define WRITE_FILE 2
#define GET_FILE_INFO 3
#define STREAM_AUDIO 4
#define DELETE_FILE 5
#define CREATE_FILE 6
#define COPY_FILE 7
#define COPY_DIR 8
#define DELETE_DIR 9
#define CREATE_DIR 10
#define STORAGE_SERVER_CONNECTION 11
#define LIST_PATHS 12
#define RECEIVE_FILE 13
#define FILE_FOUND 200
#define PING 14
#define ACK_MSG 15
#define RECEIVE_DIRECTORY 16
#define APPEND_FILE 17
#define COPY_TO_SAME_FILE 18
#define COPY_TO_SAME_DIR 19
#define BACKUP_RECEIVE 21
#define BACKUP_SEND 20
#define ACK_ASYNC 23

// Concurrency
// Accessible path structue
typedef struct {
    char path[256];     // File path (adjust size as needed)
    sem_t lock;         // Semaphore for the file
} PathLock;
typedef struct {
    PathLock paths[MAX_PATHS]; // Array of PathLock structures
    int count;                 // Number of accessible paths
    pthread_mutex_t mutex;     // Mutex to protect access to this structure
} PathLockTable;
extern PathLockTable lock_table;

int add_path_lock(PathLockTable *table, const char *path);

typedef struct storage_server{
    char IP_Addr[IP_ADDR_LEN];
    int Port_No;
    char nm_ip[IP_ADDR_LEN];
    int NM_Port;
    int Client_Port;
    int has_backup;
    struct storage_server *backup1;
    struct storage_server *backup2;
    // Rem

} storage_server;

typedef struct storage_server_info{
    char IP_Addr[IP_ADDR_LEN];
    int NS_Port_No;
    int Client_Port_No;
    char paths[MAX_PATHS*MAX_PATH_LEN];
    int ss_socket;
} storage_server_info;

typedef struct client{
    int id;
    char IP_Addr[16];
    int Port_No;
} client;

typedef struct request{
    int request_type;
    char data[MAX_REQUEST_LEN];
    char src_path[MAX_PATH_LEN];
    char src_file_dir_name[MAX_FILE_NAME];
    char dest_path[MAX_PATH_LEN];
    char ip_for_copy[IP_ADDR_LEN];  // IP of the storage server to copy the file to
    int port_for_copy;  // Port of the storage server to copy the file to
    int socket;
    int sync;
} st_request;

typedef struct client_request{
    int request_type;
    char data[MAX_REQUEST_LEN];
    char path[MAX_PATH_LEN];
    char file_or_dir_name[MAX_FILE_NAME];
    char copy_to_path[MAX_PATH_LEN];
    int client_socket;
} client_request;

typedef struct req_process {
    int client_id;
    int request_type;
    char data[MAX_DATA_LENGTH];
}req_process;

typedef struct st_response{
    int response_type;
    char message[MAX_REQUEST_LEN];
    char IP_Addr[16];
    int Port_No;
} st_response;

typedef struct file_info{
    char file_name[MAX_FILE_NAME];
    char file_path[MAX_PATH_LEN];
    int file_size;
    bool is_audio;
    bool is_dir;
    bool is_end;
    time_t last_access_time;
    time_t last_modified_time;
} file_info;

int helper_receive_backup(st_request* req,int client_port, int server_sock);
int helper_send_backup(st_request* req);

#endif