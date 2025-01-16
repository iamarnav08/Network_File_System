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

// Ports
#define PING_PORT 1200

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
#define NAMING_SERVER_IP_ADDR "10.42.0.1"

// max limits set
#define MAX_CLIENTS 100
#define MAX_DATA_LENGTH 100000
#define MAX_CONNECTIONS 120
#define MAX_FILE_NAME 30
#define MAX_REQUEST_LEN 1000
#define MAX_RESPONSE_LEN 1000
#define MAX_FILES 25
#define MAX_PATHS 100
#define IP_ADDR_LEN 16
#define MAX_PATH_LEN 50

// flags
#define STORAGE_FLAG 0
#define CLIENT_FLAG 1

// Response types
#define STORAGE_SERVER_CONNECTED 100
#define FILE_FOUND 200
#define PATH_FOUND 201
// Error codes
#define INVALID_REQUEST 400
#define INVALID_FILETYPE 402
#define FILE_NOT_FOUND 404
#define FILE_ALREADY_EXISTS 405
#define FILE_WRITE_ERROR 406
#define FILE_READ_ERROR 407
#define FILE_DELETE_ERROR 408
#define FILE_CREATE_ERROR 409
#define DELETE_DIR_ERROR 413
#define COPY_DIR_ERROR 414
#define CREATE_DIR_ERROR 415
#define FILE_COPY_ERROR 410
#define COPY_TO_PATH_INVALID 411
#define PATH_NOT_FOUND 412

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
#define RECIEVE_FILE 13
#define PING 14
#define ACK 15
#define RECIEVE_DIR 16
#define INACTIVE_STORAGE_SERVER_ACTIVATED 17
#define COPY_TO_SAME_FILE 18
#define COPY_TO_SAME_DIR 19
#define BACKUP_RECEIVE 21
#define BACKUP_SEND 20

// Structures

typedef struct {
    int socket;
    int index;
    int client_socket;
} thread_args;

typedef struct storage_server{
    int storage_server_socket;
    char IP_Addr[IP_ADDR_LEN];
    int Port_No;   // port of storage server through which communicates to the naming server
    int Client_Port; // port with which ss communicates to client and is given by NS
    
    bool is_active;
    bool is_backed_up;

    int backup_storage_server1;  // index of the 1st storage server which is the backup for this storage server
    int backupfoldernumber1;        // folder number 1,2 or 3 of the backup storage server 1 
    int backup_storage_server2;    // index of the 2nd storage server which is the backup for this storage server
    int backupfoldernumber2;    // folder number 1,2 or 3 of the backup storage server 2

    int ss_socket;
} storage_server;
typedef storage_server* ss;

typedef struct storage_server_info {
    char IP_Addr[IP_ADDR_LEN];
    int NS_Port_No;             // port with which ss is connected to naming server
    int Client_Port_No;         // port with which ss communicates to client
    char paths[MAX_PATHS*MAX_PATH_LEN];
    int ss_socket;
} storage_server_info;
typedef storage_server_info* ss_info;

typedef struct client {
    int client_id;
    char IP_Addr[IP_ADDR_LEN];
    int Port_No;
} client;

typedef struct st_request{
    int request_type;
    char data[MAX_REQUEST_LEN];
    char src_path[MAX_PATH_LEN];
    char src_file_dir_name[MAX_FILE_NAME];
    char dest_path[MAX_PATH_LEN];
    char ip_for_copy[IP_ADDR_LEN];  // IP of the storage server to connect with for copy/paste
    int port_for_copy;  // Port of the storage server to connect with for copy/paste
    int socket;
    int sync;
} st_request;
typedef st_request* request;

typedef struct req_process {
    int client_id;
    int request_type;
    char data[MAX_REQUEST_LEN];
    char src_path[MAX_PATH_LEN];
    char src_file_dir_name[MAX_FILE_NAME];
    char dest_path[MAX_PATH_LEN];
    char ip_for_copy[IP_ADDR_LEN];  // IP of the storage server to connect with for copy/paste
    int port_for_copy;  // Port of the storage server to connect with for copy/paste
    int socket;

}req_process;
typedef req_process* proc;

typedef struct st_response{
    int response_type;
    char message[MAX_REQUEST_LEN];
    char IP_Addr[IP_ADDR_LEN];
    int Port_No;
} st_response;
typedef st_response* response;

typedef struct file_info{
    char file_name[MAX_FILE_NAME];
    char file_path[MAX_PATH_LEN];
    int file_size;
    bool is_audio;
    bool is_dir;
    time_t last_access_time;
    time_t last_modified_time;
} file_info;

// Global Variables
extern client client_list[MAX_CLIENTS];
extern ss storage_server_list[100];
extern int socket_arr[MAX_CONNECTIONS][2];
extern int storage_server_count;
extern int active_storage_servers;

// Functions for handling requests
void *work_handler();
void handle_file_request(request req, int client_id);
void file_requests_to_storage_server(request req, int client_id);
void remove_paths_from_hash(int index);
void backup_dir_1(int i);
void backup_dir_2(int i);
void* backup_handler();
void* storage_server_handler(void* args);


// Hashing
#define itablesize 196613

typedef struct st_node {
    char* path;              // Path string
    int len;                 // Length of the path
    int s_index;             // Storage server index
    struct st_node* next;    // Pointer to the next node in the chain
} st_node;
typedef st_node* node;
extern node* hashtable;

node* Create_hashtable();
void Free_hashtable();
unsigned long create_hash(const char* x);
void Insert(const char* path, int s_i);
int Get(const char* path);
void Delete(const char* path);
response Print_all_paths();



// Logging 
typedef struct LogEntry {
    time_t timestamp;             // The time of the log entry
    bool isClient;                // True if the log entry is for a client, false if for a SS
    char IP_Addr[IP_ADDR_LEN];  // IP address of the client or SS
    int Port_No;               // Port number of the client or SS
    st_request request;         // Request made by the client
    int response_code;          // Response code of the request
} LogEntry;

void logMessage(bool isClient, int client_socket, st_request request, int response_code,bool fromcache);

// Cache Handling
#define CACHE_SIZE 15
typedef struct cache_entry{
    int ss_ind;
    request key;
    response value;
} cache_entry;

typedef struct my_cache{
    cache_entry cache_arr[CACHE_SIZE];
    int curr_size;
} my_cache;
extern my_cache* cache;

my_cache* initialize_cache();
void Free_cache();
void clear_cache();
bool is_cache_full();
bool is_cache_empty();
void move_to_front(int index);
void add_to_cache(request key, response value, int ss_ind);
response get_from_cache(request key);
void remove_from_cache(request key);
void delete_from_cache_ssid(int ss_id);

#endif