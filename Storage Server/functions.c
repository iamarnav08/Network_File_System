#include "functions.h"
#include"headers.h"

extern int thread_count;
extern int nm_port; // Naming Server port number
extern int client_port;
extern int storage_server_socket;
int ns_sock;
extern char nm_ip[IP_ADDR_LEN];

int register_with_naming_server(storage_server *ss, char *accessible_paths, int ss_socket)
{
    printf("ss->IP_Addr: %s\n", ss->IP_Addr);
    printf("ss->Port_No: %d\n", ss->Port_No);
    printf("ss->nm_ip: %s\n", ss->nm_ip);
    printf("ss->NM_Port: %d\n", ss->NM_Port);
    printf("accessible_paths: %s\n", accessible_paths);
    // ns_sock;
    struct sockaddr_in ns_addr;
    
    // Set up socket to connect to Naming Server
    ns_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ns_sock < 0)
    {
        perror("NS socket creation failed");
        return -1;
    }
    ns_addr.sin_family = AF_INET;
    ns_addr.sin_port = htons(ss->NM_Port);
    if (inet_pton(AF_INET, ss->nm_ip, &ns_addr.sin_addr) < 0)
    {
        perror("Invalid Naming Server IP address");
        close(ns_sock);
        return -1;
    }

    // Connect to Naming Server
    if (connect(ns_sock, (struct sockaddr *)&ns_addr, sizeof(ns_addr)) < 0)
    {
        perror("Failed to connect to Naming Server");
        close(ns_sock);
        return -1;
    }
    printf("here\n");
    char str[] = "storage_server";


    printf("Connected to Naming Server at %s:%d\n", ss->nm_ip, ss->NM_Port);
    // Send a message to the Naming Server to confirm the connection
    if (send(ns_sock, &str, sizeof(str), 0) < 0)
    {
        perror("Failed to send message to NS");
        close(ns_sock);
        return -1;
    }
    printf("Sent message to NS\n");
    // receive message from the naming server
    char buffer[1024] = {0};
    // read(ns_sock, buffer, 1024);
    recv(ns_sock, buffer, 1024, 0);
    printf("buffer:%s\n", buffer);

    // Populate storage_server_info structure
    storage_server_info ss_info;
    // strncpy(ss_info.IP_Addr, ss->IP_Addr, sizeof(ss_info.IP_Addr) - 1);
    // ss_info.IP_Addr[sizeof(ss_info.IP_Addr) - 1] = '\0'; // Ensure null termination
    strcpy(ss_info.IP_Addr, ss->IP_Addr);
    // ss_info.Port_No = ss->Port_No;
    // ss_info.NS_Port_No = NS_LISTEN_PORT;
    // ss_info.NS_Port_No = nm_port;
    // ss_info.Client_Port_No = CLIENT_LISTEN_PORT;
    ss_info.Client_Port_No = client_port;
    ss_info.NS_Port_No=ss->Port_No;
    // strncpy(ss_info.paths, accessible_paths, sizeof(ss_info.paths) - 1);
    // ss_info.paths[sizeof(ss_info.paths) - 1] = '\0'; // Ensure null termination
    strcpy(ss_info.paths, accessible_paths);
    printf("ss_info.paths: %s\n", ss_info.paths);
    ss_info.ss_socket=ss_socket;

    // Send storage_server_info to Naming Server
    if (send(ns_sock, &ss_info, sizeof(ss_info), 0) < 0)
    {
        perror("Failed to send Storage Server details to NS");
        close(ns_sock);
        return -1;
    }

    printf(GREEN_COLOR "Storage Server registered with Naming Server at %s:%d\n" RESET_COLOR, NS_IP, NS_PORT);

    // Close the socket after sending
    // close(ns_sock);
    return 0;
}

void *naming_server_listener(void *arg)
{
    pthread_t ping_thread;
    pthread_create(&ping_thread, NULL, ping_handler, NULL);
    struct sockaddr_in ns_addr;
    socklen_t ns_addr_len = sizeof(ns_addr);

    printf("Listening for Naming Server requests on ns_sock: %d...\n", ns_sock);

    while (1)
    {
        // printf("listening in loop\n");
        // Receive a st_request from the Naming Server
        st_request req;
        int bytes_received = recv(ns_sock, &req, sizeof(req), MSG_WAITALL);
        if (bytes_received < 0)
        {
            perror("Failed to receive st_request from Naming Server");
            continue;
        }
        else if (bytes_received == 0)
        {
            printf("Naming Server disconnected\n");
            break;
        }
        else
        {
            printf("Request received from Naming Server\n");
            printf("Request type: %d\n", req.request_type);

            // Handle the request in a new thread
            pthread_t ns_thread;
            if (thread_count < MAX_THREADS)
            {
                if (pthread_create(&ns_thread, NULL, work_handler, (void *)&req) < 0)
                {
                    perror("Failed to create thread for Naming Server request");
                    continue;
                }
                else
                {
                    thread_count++;
                }
            }
            else
            {
                printf("Max threads reached\n");
            }
            pthread_detach(ns_thread); // Detach the thread to allow it to clean up after itself
        }
    }

    close(ns_sock);
    pthread_exit(NULL);
}

void *client_listener(void *arg)
{
    int threads_no = 0;
    int server_sock, client_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    // int port = ((int)arg);  // Retrieve the port from the argument
    // int port = CLIENT_LISTEN_PORT;
    int port=client_port;
    printf("port from client listener:%d\n", port);

    // Create socket for listening to clients
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("Failed to create client listener socket");
        pthread_exit(NULL);
    }
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Set the socket to non-blocking mode
    int flags = fcntl(server_sock, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl F_GETFL");
        close(server_sock);
        pthread_exit(NULL);
    }
    if (fcntl(server_sock, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl F_SETFL");
        close(server_sock);
        pthread_exit(NULL);
    }

    // Set up server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    printf("port from here:%d\n", port);

    // Bind the socket to the specified port
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Client listener socket bind failed");
        close(server_sock);
        pthread_exit(NULL);
    }
    printf("Listening for client connections on port %d...\n", port);

    while (listen(server_sock, MAX_CLIENTS) >= 0)
    {
        // Accept an incoming client connection
        client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0)
        {
            // perror("Failed to accept client connection");
            continue;
        }

        printf("Client connected.\n");

        // Handle client requests

        // receive a st_request from the client
        // st_request req;
        st_request *req = malloc(sizeof(st_request));
        if (!req) {
            perror("Failed to allocate memory for request");
            close(client_sock);
            continue;
        }
        int bytes;
        int bytes_received = recv(client_sock, req, sizeof(st_request), 0);
        printf("bytes received: %d\n", bytes_received);
        printf("Request type: %d\n", req->request_type);
        printf("%s\n", req->src_path);
        printf("%s\n", req->dest_path);
        if (bytes_received < 0) {
            perror("Failed to receive st_request from client");
        } else if (bytes_received == 0) {
            printf("Client disconnected\n");
        } else {
        }
        pthread_t client_req;
        req->socket = client_sock;
        // pthread_create(&client_req, NULL, work_handler, (void *)&req);
        if (threads_no < MAX_THREADS)
        {
            pthread_create(&client_req, NULL, work_handler, (void *)req);
            threads_no++;
        }
        else
        {
            printf("Max threads reached\n");
        }
        pthread_join(client_req, NULL);
        free(req);
        // Close client socket after the interaction is done
        close(client_sock);
    }

    // Close server socket when done (will not be reached in this example)
    close(server_sock);
    pthread_exit(NULL);
}

void *work_handler(void *arg)
{
    st_request req = *((st_request *)arg);
    // printf("Printing Request type at work handler: %d\n", req.request_type);
    // printf("Data: %s\n", req.data);
    printf("request type : %d\n", req.request_type);
    printf("Path: %s\n", req.src_path);
    printf("dest path: %s\n", req.dest_path);
    printf("File or dir name: %s\n", req.src_file_dir_name);
    // printf("Copy to path: %s\n", req.dest_path);
    switch (req.request_type)
    {
    case 0:
        break;
    case READ_FILE:
        printf("Reading file at handker \n");
        // read_file(&req);
        printf("path at handler: %s\n", req.src_path);
        printf("data at handler: %s\n", req.data);
        if(read_file(&req) < 0){
            perror("Failed to read file");
        }
        // else{
        //     // send ack st-response to the naming server on nm_port
        //     st_response ack;
        //     ack.response_type=ACK_MSG;
        //     if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
        //     {
        //         perror("Failed to send ACK to Naming Server");
        //     }
        //     else
        //     {
        //         printf("ACK sent to Naming Server\n");
        //     }
        // }
        break;
    case WRITE_FILE:
        // write_file(&req);
        if(write_file(&req) < 0){
            perror("Failed to write file");
        }
        // else{
        //     // send ack st-response to the naming server on nm_port
        //     st_response ack;
        //     ack.response_type=ACK_MSG;
        //     if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
        //     {
        //         perror("Failed to send ACK to Naming Server");
        //     }
        //     else
        //     {
        //         printf("ACK sent to Naming Server\n");
        //     }
        // }
        break;
         case APPEND_FILE:
        // write_file(&req);
        if(append_file(&req) < 0){
            perror("Failed to write file");
        }
        // else{
        //     // send ack st-response to the naming server on nm_port
        //     st_response ack;
        //     ack.response_type=ACK_MSG;
        //     if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
        //     {
        //         perror("Failed to send ACK to Naming Server");
        //     }
        //     else
        //     {
        //         printf("ACK sent to Naming Server\n");
        //     }
        // }
        break;
    case GET_FILE_INFO:
        // get_file_info(&req);
        if(get_file_info(&req) < 0){
            perror("Failed to get file info");
        }
        // else{
        //     // send ack st-response to the naming server on nm_port
        //     st_response ack;
        //     ack.response_type=ACK_MSG;
        //     if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
        //     {
        //         perror("Failed to send ACK to Naming Server");
        //     }
        //     else
        //     {
        //         printf("ACK sent to Naming Server\n");
        //     }
        // }
        break;
    case STREAM_AUDIO:
        // printf("Streaming audio at handler\n");
        // printf("path at handler: %s\n", req.src_path);
        // stream_audio(&req);
        if(stream_audio(&req) < 0){
            perror("Failed to stream audio");
        }
        // else{
        //     // send ack st-response to the naming server on nm_port
        //     st_response ack;
        //     ack.response_type=ACK_MSG;
        //     if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
        //     {
        //         perror("Failed to send ACK to Naming Server");
        //     }
        //     else
        //     {
        //         printf("ACK sent to Naming Server\n");
        //     }
        // }
        break;
    case COPY_FILE:
        if(copy_file(&req) < 0){
            perror("Failed to copy file");
        }
        else{
            // send ack st-response to the naming server on nm_port
            st_response ack;
            ack.response_type=ACK_MSG;
            // printf('%d\n',ns_sock);
            if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
            {
                perror("Failed to send ACK to Naming Server");
            }
            else
            {
                printf("ACK sent to Naming Server\n");
            }
        }
        break;
    case DELETE_FILE:
        // delete_file_or_directory(&req);
        if(delete_file_or_directory(&req) < 0){
            perror("Failed to delete file");
        }
        else{
            // send ack st-response to the naming server on nm_port
            st_response ack;
            ack.response_type=ACK_MSG;
            if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
            {
                perror("Failed to send ACK to Naming Server");
            }
            else
            {
                printf("ACK sent to Naming Server\n");
            }
        }
        break;
    case CREATE_FILE:
        // create_file_or_directory(&req);
        if(create_file_or_directory(&req) < 0){
            perror("Failed to create file");
            
        }
        else{
            // send ack st-response to the naming server on nm_port
            st_response ack;
            ack.response_type=ACK_MSG;
            if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
            {
                perror("Failed to send ACK to Naming Server");
            }
            else
            {
                printf("ACK sent to Naming Server\n");
            }
        }
        break;
    case COPY_DIR:
        printf("Copying directory at handler\n");
        if(copy_directory(&req) < 0){
            perror("Failed to copy directory");
        }
        else{
            // send ack st-response to the naming server on nm_port
            st_response ack;
            ack.response_type=ACK_MSG;
            if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
            {
                perror("Failed to send ACK to Naming Server");
            }
            else
            {
                printf("ACK sent to Naming Server\n");
            }
        }
        break;
    case DELETE_DIR:
        // delete_file_or_directory(&req);
        if(delete_file_or_directory(&req) < 0){
            perror("Failed to delete directory");
        }
        else{
            // send ack st-response to the naming server on nm_port
            st_response ack;
            ack.response_type=ACK_MSG;
            if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
            {
                perror("Failed to send ACK to Naming Server");
            }
            else
            {
                printf("ACK sent to Naming Server\n");
            }
        }
        break;
    case CREATE_DIR:
        // create_file_or_directory(&req);
        printf("Creating directory at handler\n");
        if(create_file_or_directory(&req) < 0){
            perror("Failed to create directory");
        }
        else{
            // send ack st-response to the naming server on nm_port
            st_response ack;
            ack.response_type=ACK_MSG;
            if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
            {
                perror("Failed to send ACK to Naming Server");
            }
            else
            {
                printf("ACK sent to Naming Server\n");
            }
        }
        break;
    case RECEIVE_FILE:
        {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_sock = accept(storage_server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_sock < 0)
            {
                perror("Failed to accept connection");
                break;
            }
            receive_file(&req, client_sock);
            close(client_sock);
        }
        break;
    case RECEIVE_DIRECTORY:
        {
            // printf("Receiving directory at handler\n");
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_sock = accept(storage_server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_sock < 0)
            {
                perror("Failed to accept connection");
                break;
            }
            receive_directory(&req, client_sock);
            close(client_sock);
        }
        break;
    case COPY_TO_SAME_DIR:
        if(copy_local_directory(&req) < 0){
            perror("Failed to copy to same directory");
        }
        else{
            // send ack st-response to the naming server on nm_port
            st_response ack;
            ack.response_type=ACK_MSG;
            if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
            {
                perror("Failed to send ACK to Naming Server");
            }
            else
            {
                printf("ACK sent to Naming Server\n");
            }
        }
        break;
    case COPY_TO_SAME_FILE:
        if(copy_local_file(&req) < 0){
            perror("Failed to copy to same file");
        }
        else{
            // send ack st-response to the naming server on nm_port
            st_response ack;
            ack.response_type=ACK_MSG;
            if (send(ns_sock, &ack, sizeof(ack), 0) < 0)
            {
                perror("Failed to send ACK to Naming Server");
            }
            else
            {
                printf("ACK sent to Naming Server\n");
            }
        }
        break;
    case BACKUP_RECEIVE:
        {
            if(helper_receive_backup(&req,client_port,storage_server_socket)){

            }
        }
        break;

    case BACKUP_SEND:
        {
            helper_send_backup(&req);
        }
        break;
    default:
        break;
    }
}

void* ping_handler(void *arg){
    // create a socket for ping on NS_IP and port 4000
    usleep(1000);
    int ping_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(ping_sock == -1){
        perror("Ping socket creation failed");
        exit(1);
    }
    // make the port reusable
    int opt = 1;
    if (setsockopt(ping_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(ping_sock);
        exit(EXIT_FAILURE);
    }
    // bind the socket to the naming server's IP and port   
    struct sockaddr_in ping_addr;
    ping_addr.sin_family = AF_INET;
    ping_addr.sin_port = htons(PING_PORT);
    printf("nm_ip for ping: %s\n", nm_ip);
    if(inet_pton(AF_INET, nm_ip, &ping_addr.sin_addr) < 0){
        perror("Invalid destination IP address");
        return NULL;
    }
    if(connect(ping_sock, (struct sockaddr*)&ping_addr, sizeof(ping_addr)) < 0){
        perror("Failed to connect to Naming Server for ping\n");
        close(ping_sock);
        return NULL;
    }

    while(1){
        // printf("Ping handler\n");
        st_response ping;
        ping.response_type = PING;
        // printf("ping.response_type: %d\n", ping.response_type);
        send(ping_sock, &ping, sizeof(st_response), 0);
        sleep(10);
    }
}