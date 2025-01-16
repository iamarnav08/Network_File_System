#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include "headers.h"

extern int storage_server_socket;

int register_with_naming_server(storage_server *ss, char* accessible_paths, int ss_socket);

void *naming_server_listener(void *arg);
void *client_listener(void *arg);
void* ping_handler(void *arg);

int get_ip_and_port(char* ip_addr, int* port);
extern int ext_storage_server_port;

void* work_handler(void *arg);

//Naming Server Functions
int delete_file_or_directory(st_request* req);
int delete_file_or_directory_recursive(const char* path);
int copy_file(st_request* req);
int copy_directory(st_request* req);
int create_file_or_directory(st_request* req);
int receive_file(st_request* req, int storage_server_socket);
int receive_directory(st_request* req, int storage_server_socket);
int copy_local_file(st_request* req);
int copy_local_directory(st_request* req);

//Client Functions
int read_file(st_request* req);
int write_file(st_request* req);
int append_file(st_request *req);
int get_file_info(st_request* req);
int stream_audio(st_request* req);

// Functions for accessible paths
void traverse_directory(const char *base_dir, const char *relative_dir, char **result, size_t *result_size, size_t *len);
char* get_all_files_and_dirs_recursive(const char *base_dir);

#endif