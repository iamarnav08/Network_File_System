#include"headers.h"

ss storage_server_list[100];
extern node* hashtable;
int socket_arr[MAX_CONNECTIONS][2];
int storage_server_count = 0;
int active_storage_servers = 0;

void* handle_client_process(void *arg) {
    // handle the client request
    proc n = (proc)arg;
    int client_id = n->client_id;
    request req = (request)malloc(sizeof(st_request));
    req->request_type = n->request_type;
    strcpy(req->data, n->data);
    strcpy(req->src_path, n->src_path);
    strcpy(req->dest_path, n->dest_path);
    strcpy(req->src_file_dir_name, n->src_file_dir_name);
    strcpy(req->ip_for_copy, n->ip_for_copy);
    req->port_for_copy = n->port_for_copy;
    req->socket = n->socket;
    if(req->request_type == READ_FILE || req->request_type == WRITE_FILE || req->request_type == GET_FILE_INFO || req->request_type == STREAM_AUDIO) {
        handle_file_request(req, client_id);
    }
    else if(req->request_type == CREATE_FILE || req->request_type == DELETE_FILE || req->request_type == COPY_FILE || req->request_type == CREATE_DIR || req->request_type == DELETE_DIR || req->request_type == COPY_DIR || req->request_type == COPY_TO_SAME_FILE || req->request_type == COPY_TO_SAME_DIR) {
        file_requests_to_storage_server(req, client_id);
    }
    else if(req->request_type == LIST_PATHS)
    {   
        response r;
        printf("Printing all paths\n");
        r = Print_all_paths();
        // tokenise the r->message and send it to the client
        // printf("%s\n", r->message);
        char p1[10000];
        strcpy(p1, r->message);
        r->response_type = ACK;
        // char *token = strtok(p1, ";");
        // while (token != NULL) {
        //     printf("%s\n", token);
        //     token = strtok(NULL, ";");
        // }
        printf("Paths: %s\n", r->message);
        send(client_id, r, sizeof(st_response), 0);
        logMessage(CLIENT_FLAG, client_id, *req, r->response_type,0);
    }
    free(req);
}

void* client_handler(void *arg)
{
    int client_socket = *(int *)arg;
    while(1)
    {
        request req = (request)malloc(sizeof(st_request));
        bzero(req, sizeof(st_request));
        int res = recv(client_socket, req, sizeof(st_request), MSG_WAITALL);
        if(res < 0){
            free(req);
            // perror("Error in receiving request from client");
            printf("error recieving");
            break;
            // exit(1);
        } else if(res == 0){
            free(req);
            printf("Client disconnected\n");
            socket_arr[client_socket][0] = -1; 
            close(client_socket);
            break;
        }
        int req_valid = 1;
        if(req->request_type == -1)
        {
            free(req);
            req->request_type = 0;
            break;
        }
        if(req_valid)        
        {
            pthread_t process;
            proc n = (proc)malloc(sizeof(req_process));
            n->client_id = client_socket;
            n->request_type = req->request_type;
            strcpy(n->data, req->data);
            strcpy(n->src_path, req->src_path);
            strcpy(n->dest_path, req->dest_path);
            strcpy(n->src_file_dir_name, req->src_file_dir_name);
            strcpy(n->ip_for_copy, req->ip_for_copy);
            n->port_for_copy = req->port_for_copy;
            n->socket = client_socket;

            response res = get_from_cache(req);
            if(res != NULL){
                // printf("%s\n", res->message);
                // printf("%s\n", res->IP_Addr);
                // printf("%d\n", res->Port_No);
                send(client_socket, res, sizeof(st_response), 0);
                logMessage(CLIENT_FLAG, client_socket, *req, res->response_type,1);
                free(req);
                continue;
            }
            pthread_create(&process, NULL, &handle_client_process, (void *)n);
            // join the thread &process
            pthread_join(process, NULL);
        } else{
            // log the invalid request
            logMessage(CLIENT_FLAG, client_socket, *req, INVALID_REQUEST,0);
        }
        free(req);
    }
}


int check_if_exists(char *ip, int clientport, int socket, int index)
{
    for(int i = 0; i < storage_server_count; i++)
    {
        if(strcmp(storage_server_list[i]->IP_Addr, ip) == 0 && storage_server_list[i]->Client_Port == clientport)
        {
            socket_arr[index][0] = -1;
            storage_server_list[i]->is_active = true;
            storage_server_list[i]->storage_server_socket = socket;
            socket_arr[i][1] = STORAGE_FLAG;
            socket_arr[i][0] = socket;
            return i;
        }
    }
    return -1;
}

void *work_handler(){
    // create a socket for the naming server
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1){
        perror("Socket creation failed");
        exit(1);
    }
    // make the port reusable
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    // bind the socket to the naming server's IP and port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(NS_PORT);
    server_addr.sin_addr.s_addr = inet_addr(NS_IP);
    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("Binding to port failed");
        exit(1);
    }
    // listen for incoming connections
    if(listen(server_socket, MAX_CONNECTIONS) == -1){
        perror("Listening on port failed");
        exit(1);
    }
    // accept incoming connections
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    // initialize the socket array
    for(int i = 0; i < MAX_CONNECTIONS; i++){
        socket_arr[i][0] = -1;
        socket_arr[i][1] = -1;
    }
    while(1){
        // handle multiple clients
        for(int i = 0; i < MAX_CONNECTIONS; i++){
            if(socket_arr[i][0] == -1){
                socket_arr[i][0] = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
                if(socket_arr[i][0] == -1){
                    perror("Accepting connection failed");
                    exit(1);
                }
                // recieve a message from the client or storage server to determine the type of connection
                char buffer[1024] = {0};
                read(socket_arr[i][0], buffer, 1024);
                if(strcmp(buffer, "client") == 0)
                {
                    // send a message to the client to confirm the connection 
                    send(socket_arr[i][0], "Hello client server", strlen("Hello client server"), 0);
                    socket_arr[i][1] = CLIENT_FLAG;
                    printf("Client connected\n");
                    // create a new thread to handle the client request
                    pthread_t client_thread;
                    pthread_create(&client_thread, NULL, &client_handler, (void *)&socket_arr[i][0]);
                    // pthread_join(client_thread, NULL);
                }

                else if(strcmp(buffer, "storage_server") == 0){
                    // printf("Storage server %d connected\n", i);
                    // send a message to the storage server to confirm the connection
                    send(socket_arr[i][0], "Hello storage server", strlen("Hello storage_server"), 0);
                    socket_arr[i][1] = STORAGE_FLAG;

                    // recieve the storage server's IP address and port number
                    ss_info s = (ss_info)malloc(sizeof(storage_server_info));
                    ssize_t bytes_received = recv(socket_arr[i][0], s, sizeof(storage_server_info), MSG_WAITALL);
                    if (bytes_received <= 0) {
                        perror("Receiving storage server info failed");
                        free(s);
                        close(socket_arr[i][0]);
                        continue;
                    }
                    //  else if (bytes_received < sizeof(storage_server_info)) {
                    //     fprintf(stderr, "Incomplete data received\n");
                    //     free(s);
                    //     close(socket_arr[i][0]);
                    //     continue;
                    // }
                    // print the contents of s
                    int c = check_if_exists(s->IP_Addr, s->Client_Port_No, socket_arr[i][0], i);
                    thread_args *args = (thread_args *)malloc(sizeof(thread_args));
                    if(c!=-1)
                    {
                        printf("Inactive Storage server activated\n");
                        args->socket = socket_arr[c][0];
                        args->index = c;
                        request req = (request)malloc(sizeof(st_request));
                        req->request_type = INACTIVE_STORAGE_SERVER_ACTIVATED;
                        storage_server_list[c]->is_active = true;
                        // if(storage_server_list[c]->is_backed_up == false)
                        // {
                        //     char *token = strtok(s->paths, ";");
                        //     while (token != NULL) {
                        //         // printf("%s\n", token);
                        //         // printf("Hash: %lu\n", create_hash(token));
                        //         Insert(token, c);
                        //         // printf("%d\n", Get(token));
                        //         token = strtok(NULL, ";");
                        //     }
                        // }
                            char *token = strtok(s->paths, ";");
                            while (token != NULL) {
                                // printf("%s\n", token);
                                // printf("Hash: %lu\n", create_hash(token));
                                if(Get(token) == -1){
                                    Insert(token, c);
                                }
                                // printf("%d\n", Get(token));
                                token = strtok(NULL, ";");
                            }                  
                        logMessage(STORAGE_FLAG, socket_arr[c][0], *req, STORAGE_SERVER_CONNECTED,0);
                        free(req);
                    }
                    else
                    {
                        clear_cache();
                        printf("Storage server IP: %s\n", s->IP_Addr);
                        printf("Storage server Port to NS: %d\n", s->NS_Port_No);
                        printf("Storage server Port to Client: %d\n", s->Client_Port_No);
                        printf("Storage server Paths:\n");

                        char *token = strtok(s->paths, ";");
                        while (token != NULL) {
                            printf("%s\n", token);
                            // printf("Hash: %lu\n", create_hash(token));
                            Insert(token, storage_server_count);
                            // printf("%d\n", Get(token));
                            token = strtok(NULL, ";");
                        }
                        
                        storage_server_list[storage_server_count] = (ss)malloc(sizeof(storage_server));
                        storage_server_list[storage_server_count]->storage_server_socket = socket_arr[i][0];
                        strcpy(storage_server_list[storage_server_count]->IP_Addr, s->IP_Addr);
                        storage_server_list[storage_server_count]->Port_No = s->NS_Port_No;
                        storage_server_list[storage_server_count]->Client_Port = s->Client_Port_No;
                        storage_server_list[storage_server_count]->is_active = true;
                        storage_server_list[storage_server_count]->ss_socket = s->ss_socket;
                        
                        storage_server_count++;
                        request req = (request)malloc(sizeof(st_request));
                        req->request_type = STORAGE_SERVER_CONNECTION;
                        logMessage(STORAGE_FLAG, socket_arr[i][0], *req, STORAGE_SERVER_CONNECTED,0);
                        free(req);
                        // create a new thread to handle the requests sent to storage server
                        args->socket = socket_arr[i][0];
                        args->index = i;
                        // printf("index %d\n", i);
                    }
                    active_storage_servers++;
                    free(s);
                    pthread_t storage_thread;
                    pthread_create(&storage_thread, NULL, &storage_server_handler, (void *)args);
                }
            }
        }
    }
}