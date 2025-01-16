#include "headers.h"
#define NS_PORT1 4000
// ack example;
int connect_with_ss(char *ipaddress, int port)
{
    printf("%s %d\n", ipaddress, port);
    // return 0;
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in *server_address = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(port);
    server_address->sin_addr.s_addr = inet_addr(ipaddress);

    int x = connect(client_socket, (struct sockaddr *)server_address, sizeof((*server_address)));
    if (x == -1)
    {
        perror("Connection failed");
        return 0;
    }
    return client_socket;
    // example.client_socket=client_socket;
    // strcpy(example.ns_ip,ipaddress);
}

int connect_with_ns()
{
    // int index = 0;
    // while (client_socket[index] != -1)
    //     index++;
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    // client_socket[index] = client_sock;
    struct sockaddr_in *server_address = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(NS_PORT);
    server_address->sin_addr.s_addr = inet_addr(NS_IP);
    if (connect(client_sock, (struct sockaddr *)server_address, sizeof(struct sockaddr)) < 0)
    {
        perror("Connection failed");
        return -1;
        // continue;
    }
    send(client_sock, "client", 6, 0);
    char buffer[50];
    int bytes = recv(client_sock, buffer, sizeof(buffer), 0);
    if (bytes < 0)
    {
        perror("Receive failed");
        free(server_address); // Free allocated memory before returning
        close(client_sock);
        return -1;
    }
    buffer[bytes] = '\0';
    // printf("%s\n", buffer);
    free(server_address);
    // printf("%d\n",client_sock);
    return client_sock;
}

// ack *connect_with_ns_write()
// {
//     return nullptr;
// }

// ack example;
// ack *connect_with_ns_write()
// {
//     int client_socket;
//     client_socket = socket(AF_INET, SOCK_STREAM, 0);
//     struct sockaddr_in *server_address = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
//     server_address->sin_family = AF_INET;
//     server_address->sin_port = htons(NS_PORT);
//     server_address->sin_addr.s_addr = inet_addr(NS_IP);
//     if (connect(client_socket, (struct sockaddr *)server_address, sizeof(struct sockaddr)) < 0)
//     {
//         perror("Connection failed");
//         return NULL;
//         // continue;
//     }
//     send(client_socket, "client", 6, 0);
//     char buffer[50];
//     int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
//     if (bytes < 0)
//     {
//         perror("Receive failed");
//         free(server_address); // Free allocated memory before returning
//         close(client_socket);
//         return NULL;
//     }
//     buffer[bytes] = '\0';
//     printf("%s\n", buffer);
//     free(server_address);
//     int client_socket1 = socket(AF_INET, SOCK_STREAM, 0);
//     struct sockaddr_in *server_address1 = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
//     server_address1->sin_family = AF_INET;
//     server_address1->sin_port = htons(NS_PORT1);
//     server_address1->sin_addr.s_addr = inet_addr(NS_IP);

//     int y = connect(client_socket, (struct sockaddr *)server_address, sizeof((*server_address)));
//     if (y == -1)
//     {
//         perror("Connection failed");
//         return NULL;
//     }
//     example.client_socket = client_socket;
//     example.ns_socket = client_socket1;
//     // return &example;
//     return &example;
// }