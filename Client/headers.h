#ifndef __HEADERS_H__
#define __HEADERS_H__

#define LINUX

// =========================== Header files ===========================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/select.h>
#include<ctype.h>

#define MAX_DATA_LENGTH 1000    
#define MAX_FILES 10            
#define MAX_PATH_LEN 50         
#define NFS_SERVER_PORT_NO 2000 
#define MY_NFS_PORT_NO 3000     
#define MY_CLIENT_PORT_NO 4000  
#define MY_IP "0.0.0.0"         
#define NS_PORT 3000            
#define NS_IP "192.168.39.62"    

#define RED_COLOR "\033[0;31m"
#define GREEN_COLOR "\033[0;32m"
#define BLUE_COLOR "\033[0;34m"
#define YELLOW_COLOR "\033[0;33m"
#define CYAN_COLOR "\033[0;36m"
#define ORANGE_COLOR "\e[38;2;255;85;0m"
#define RESET_COLOR "\033[0m"

#define ACK 15

#define FILE_COPY_ERROR 409
#define FOLDER_COPY_ERROR 410
#define DELETE_FILE_ERROR
#define DELETE_FOLER_ERROR
#define WRITE_REQ 2
#define READ_REQ 1
#define DELETE_REQ 6
#define CREATE_REQ 7
#define COPY_FILE 7
#define STREAM_AUDIO 4
#define RETRIEVE_INFO 3
#define CREATE_FILE 6
#define CREATE_FOLDER 10
#define DELETE_FILE 5
#define DELETE_FOLDER 9
#define COPY_FOLDER 8

#define MAX_CLIENTS 100
 
#define ACK_ASYNC 23
#define FILE_NOT_FOUND 404
#define INVALID_FILETYPE 402
#define SERVER_NOT_FOUND 403

#define MAX_FILE_NAME 30
#define MAX_REQUEST_LEN 1000
#define MAX_PATHS 100
typedef struct st_request
{
    int request_type;
    char data[MAX_REQUEST_LEN];
    char src_path[MAX_PATH_LEN];
    char src_file_dir_name[MAX_FILE_NAME];
    char dest_path[MAX_PATH_LEN];
    char ip_for_copy[16]; // IP of the storage server to copy the file to
    int port_for_copy;             // Port of the storage server to copy the file to
    int socket;
    int sync;
} st_request;
typedef st_request *request;

typedef struct st_response
{
    int response_type;
    char message[MAX_DATA_LENGTH];
    char IP_Addr[16];
    int Port_No;
    // char * response;
} st_response;
typedef st_response *response;

extern int client_socket[MAX_CLIENTS];

void create_operation(char *path, char *name, int macro);
int connect_with_ss(char *ipaddress, int port);
void communicate_with_ss(char *ipaddress, int port, char *path);
int connect_with_ns();
// ack* connect_with_ns_write();   
void reading_operation(char *path);
void communicate_with_ss_write(char *ipaddress, int port, char *path,int f);
void writing_append_operation(char *path, int f);
void communicate_with_ss_info(char *ipaddress, int port, char *path);
void info(char *path);
void delete_operation(char *path, char *name, int macro);
void copy_operation(int req_type, char *path1, char *path2);
void list();
void communicate_with_ss_stream(char *ipaddress, int port, char *path);
void communicate_with_ss_backup(char *ipaddress, int port, char *path);
void stream(char *path);
void communicate_with_ss_stream(char *ipaddress, int port, char *path);
// void man();

#endif
