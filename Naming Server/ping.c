#include"headers.h"

void* storage_server_handler(void* args)
{
    if (args == NULL) {
        fprintf(stderr, "Error: args is NULL\n");
        return NULL;
    }
    // help me create a new socket for the naming server via which it recieves ping messages from the storage server
    thread_args *t_args = (thread_args *)args;
    if (t_args == NULL) {
        fprintf(stderr, "Error: t_args is NULL\n");
        return NULL;
    }
    int storage_socket = t_args->socket;
    int index = t_args->index;
    int naming_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(naming_server_socket == -1){
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    int opt = 1;
    if (setsockopt(naming_server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(naming_server_socket);
        pthread_exit(NULL);
    }

    // Set the socket to non-blocking mode
    // int flags = fcntl(naming_server_socket, F_GETFL, 0);
    // if (flags == -1) {
    //     perror("fcntl F_GETFL");
    //     close(naming_server_socket);
    //     exit(EXIT_FAILURE);
    // }

    // if (fcntl(naming_server_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
    //     perror("fcntl F_SETFL");
    //     close(naming_server_socket);
    //     exit(EXIT_FAILURE);
    // }
    
    struct sockaddr_in naming_server_addr;
    naming_server_addr.sin_family = AF_INET;
    naming_server_addr.sin_port = htons(PING_PORT);
    naming_server_addr.sin_addr.s_addr = INADDR_ANY;
    // if(inet_pton(AF_INET, NS_IP, &naming_server_addr.sin_addr) < 0){
    //     perror("Invalid destination IP address");
    //     exit(1);
    // }

    if(bind(naming_server_socket, (struct sockaddr *)&naming_server_addr, sizeof(naming_server_addr)) == -1){
        perror("Binding to port failed");
        close(naming_server_socket);
        exit(1);
    }

    if(listen(naming_server_socket, MAX_CONNECTIONS) == -1){
        perror("Listening on port failed");
        close(naming_server_socket);
        exit(1);
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket = accept(naming_server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
    printf("packet accepted\n");
    if(client_socket == -1){
        perror("Accepting connection failed");
        close(naming_server_socket);
        exit(1);
    }

    printf("Storage server %d connected to naming server\n", index);
    while (1) {
        response res = (response)malloc(sizeof(st_response));
        memset(res, 0, sizeof(st_response));
        int r = recv(client_socket, res, sizeof(st_response), MSG_WAITALL);
        // printf("%d\n", res->response_type);
        if(r < 0){
            free(res);
            printf("error recieving");
            break;
        } 
        else if(r==0)
        {
            // handle the disconnection of the storage server
            printf("Storage server disconnected\n");
            if(!storage_server_list[index]->is_backed_up)
            {
                remove_paths_from_hash(index);
            }

            // segfault
            delete_from_cache_ssid(index);
            free(res);
            socket_arr[index][0] = -1;
            printf("index: %d\n", index);
            storage_server_list[index]->storage_server_socket = -1;
            storage_server_list[index]->is_active = false;
            active_storage_servers--;
            close(storage_socket);
            break;
        }
        else{
            // printf("Storage server %d is active\n", res->Port_No);
            // printf("request type: %d\n", res->response_type);
        }
    }
    // close(naming_server_socket);
    close(client_socket);
}