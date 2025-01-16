#include "functions.h"

int thread_count;
int nm_port;    // Naming Server port number
int client_port;
int storage_server_socket;
char nm_ip[IP_ADDR_LEN];
int ext_storage_server_port;
PathLockTable lock_table;

void init_path_lock_table(PathLockTable *table) {
    table->count = 0;
    pthread_mutex_init(&table->mutex, NULL);
    for (int i = 0; i < table->count; i++) {
        sem_init(&table->paths[i].lock, 0, 1); // Initialize semaphore to 1
    }
}
int add_path_lock(PathLockTable *table, const char *path) {
    printf("Adding path to hash_table: %s\n", path);
    pthread_mutex_lock(&table->mutex);

    if (table->count >= MAX_PATHS) {
        pthread_mutex_unlock(&table->mutex);
        return -1; // Table full
    }

    // Add the path and initialize its semaphore
    strncpy(table->paths[table->count].path, path, MAX_PATH_LEN - 1);
    table->paths[table->count].path[MAX_PATH_LEN - 1] = '\0'; // Null-terminate
    sem_init(&table->paths[table->count].lock, 0, 1);         // Semaphore for the path
    table->count++;

    pthread_mutex_unlock(&table->mutex);
    return 0; // Success
}
int add_paths_to_table(const char *paths_str, PathLockTable *table) {
    char *paths_copy = strdup(paths_str);  // Create a modifiable copy of the input string
    if (!paths_copy) {
        perror("Failed to allocate memory");
        return -1;
    }

    char *token = strtok(paths_copy, ";");
    while (token) {
        printf("Adding path: %s\n", token);
        if (add_path_lock(table, token) != 0) {
            fprintf(stderr, "Failed to add path: %s\n", token);
            free(paths_copy);
            return -1;
        }
        token = strtok(NULL, ";");
    }

    free(paths_copy);
    return 0; // Success
}


int main(int argc, char *argv[]){
    thread_count=0;
    storage_server ss1;
    char ip[IP_ADDR_LEN];
    // char nm_ip[IP_ADDR_LEN];
    int ss_port_no;
    // get_ip_address(ip);
    if(get_ip_and_port(ip, &ss_port_no)==-1){
        perror("Failed to get ip and port no of storage_server\n");
        return(EXIT_FAILURE);
    }
    printf("ip: %s\n", ip);
    // printf("Enter port no for connecting with naming server: ");
    // scanf("%d", &nm_port);
    printf("Enter port no for connecting with clients: ");
    scanf("%d", &client_port);
    
    printf("Enter port number for communicating with other storage servers: "); 
    scanf("%d", &ext_storage_server_port);

    // create socket for communicating with other storage servers
    struct sockaddr_in storage_server_addr;
    storage_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (storage_server_socket < 0)
    {
        perror("Failed to create storage server socket");
        return -1;
    }
    storage_server_addr.sin_family = AF_INET;
    storage_server_addr.sin_port = htons(ext_storage_server_port);
    storage_server_addr.sin_addr.s_addr = INADDR_ANY;
    if (inet_pton(AF_INET, ip, &storage_server_addr.sin_addr) < 0) {
        perror("Invalid destination IP address");
        return -1;
    }
    if (bind(storage_server_socket, (struct sockaddr *)&storage_server_addr, sizeof(storage_server_addr)) < 0)
    {
        perror("Failed to bind storage server socket");
        return -1;
    }
    if (listen(storage_server_socket, 10) < 0)
    {
        perror("Failed to listen on storage server socket");
        return -1;
    }

    // printf("Storage server listening on port %d\n", ext_storage_server_port);


    strncpy(ss1.IP_Addr, ip, sizeof(ss1.IP_Addr));
    // ss1.Port_No=ss_port_no;
    // ss1.Port_No=NS_LISTEN_PORT;
    ss1.Port_No=ext_storage_server_port;
    printf("ss1.Port_No: %d\n", ss1.Port_No);
    // ss1.NM_Port=atoi(argv[1]);
    printf("%s", argv[1]);
    strcpy(nm_ip, argv[1]);
    strcpy(ss1.nm_ip, nm_ip);
    ss1.NM_Port=atoi(argv[2]);

    // char accessible_paths[MAX_PATHS*MAX_PATH_LEN];
    char* accessible_paths;
    const char* base_dir="./main";
    accessible_paths=get_all_files_and_dirs_recursive(base_dir);
    int k;
    // }
    printf("%s\n", accessible_paths);

    if(register_with_naming_server(&ss1, accessible_paths, storage_server_socket)<0){
        fprintf(stderr, "Failed to register with Naming Server\n");
        exit(1);
    }

    printf("%s\n", accessible_paths);
    init_path_lock_table(&lock_table);
    if(add_paths_to_table(accessible_paths, &lock_table)<0){
        fprintf(stderr, "Failed to add paths to array\n");
        exit(1);
    }

    // Starting threads for listening to Naming Server and Clients
    pthread_t ns_thread, client_thread, ping_thread;
    pthread_create(&ns_thread, NULL, naming_server_listener, NULL);
    pthread_create(&client_thread, NULL, client_listener, NULL);
    

    // Wait for threads to finish
    // pthread_join(ns_thread, NULL);
    pthread_join(ns_thread, NULL);
    pthread_join(client_thread, NULL);

    return 0;
}