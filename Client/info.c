#include "headers.h"

void communicate_with_ss_info(char *ipaddress, int port, char *path)
{
    int client_socket = connect_with_ss(ipaddress, port);
    st_request readerpacket;
    readerpacket.request_type = RETRIEVE_INFO;
    strcpy(readerpacket.src_path, path);
    int bytes_sent = send(client_socket, &readerpacket, sizeof(st_request), 0);
    if (bytes_sent == -1)
    {
        perror("Send failed");
    }

    // Reader Packet sent
    char buffer[100];
    request req = (request)malloc(sizeof(st_request));
    int bytes;
    if ((bytes = recv(client_socket, req, sizeof(st_request), 0)) < 0)
    {
        perror("Receiving data failed");
        // continue;
    }
    printf("%d\n",req->request_type);
    if (req->request_type == ACK)
    {
        printf("%s\n", req->data);
    }
    else
    {
        printf(RED_COLOR "Failed to retrieve Data : %s\n" RESET_COLOR, req->data);
    }
    free(req);
    close(client_socket);
}

void info(char *path)
{
    int client_socket = connect_with_ns();
    st_request readerpacket;
    readerpacket.request_type = RETRIEVE_INFO;
    strcpy(readerpacket.src_path, path);
    ssize_t bytes_sent = send(client_socket, &readerpacket, sizeof(st_request), 0);
    if (bytes_sent == -1)
    {
        perror("Send failed");
    }
    st_response response;
    ssize_t bytes_received = recv(client_socket, &response, sizeof(st_request), 0);
    int port;
    char *ipaddress = (char *)malloc(16 * sizeof(char));
    if (bytes_received == -1)
    {
        perror("Receive failed");
    }
    else
    {
        if (response.response_type == FILE_NOT_FOUND)
        {
            printf(RED_COLOR "File/Directory Not Found \n" RESET_COLOR); // Error Not Found File
            return;
        }
        else if (response.response_type == INVALID_FILETYPE)
        {
            printf(RED_COLOR "Invalid file type\n" RESET_COLOR);
            close(client_socket);
            return;
        }
        else
        {
            port = response.Port_No;
            strcpy(ipaddress, response.IP_Addr);
        }
    }
    close(client_socket);
    communicate_with_ss_info(ipaddress, port, path);
}
