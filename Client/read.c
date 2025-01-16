#include "headers.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

void communicate_with_ss(char *ipaddress, int port, char *path)
{
    printf("Connecting to server at %s:%d\n", ipaddress, port);

    int client_socket = connect_with_ss(ipaddress, port);
    if (client_socket < 0)
    {
        perror("Connection failed");
        return;
    }

    st_request readerpacket;
    readerpacket.request_type = 1; // Assuming 1 is the request type for READ_REQ
    strcpy(readerpacket.src_path, path);
    readerpacket.socket = client_socket;

    if (send(client_socket, &readerpacket, sizeof(st_request), 0) < 0)
    {
        perror("Send failed");
        close(client_socket);
        return;
    }

    printf("Request sent to server: %s\n", path);

    st_request req;
    memset(&req, 0, sizeof(st_request));

    // Allocate buffer to concatenate all received data
    char buffer[1000000];              // Assuming enough space for all data
    memset(buffer, 0, sizeof(buffer)); // Initialize the buffer to empty

    int total_bytes_received = 0;
    int bytes_received;

    while ((bytes_received = recv(client_socket, &req, sizeof(st_request),0)) > 0)
    {
        // Handle specific request type
        if (req.request_type == 407)
        {
            printf(RED_COLOR "%s" RESET_COLOR, req.data);
            break;
        }

        if (req.request_type == 15)
        {
            printf(GREEN_COLOR "ACK received. Stopping reception.\n" RESET_COLOR);
            break;
        }

        // Debugging: Check received bytes and content
        printf("Bytes received: %d\n", bytes_received);
        printf("Data: '%.*s'\n", (int)bytes_received, req.data);

        // Ensure null-termination for safety
        if (bytes_received > 0 && bytes_received < sizeof(req.data))
        {
            req.data[bytes_received] = '\0';
        }

        strcat(buffer, req.data);

        memset(&req, 0, sizeof(st_request));
    }

    // Print the entire concatenated data after the loop
    printf("Total received data: %s %ld\n", buffer,strlen(buffer));

    close(client_socket);
}

void reading_operation(char *path)
{
    int client_socket = connect_with_ns();
    printf("%s\n", path);
    st_request readerpacket;
    readerpacket.request_type = READ_REQ;
    strcpy(readerpacket.src_path, path);
    int bytes_sent = send(client_socket, &readerpacket, sizeof(st_request), 0);
    if (bytes_sent < 0)
    {
        perror("Send failed");
    }
    st_response *response = (st_response *)malloc(sizeof(st_response));
    int bytes_received = recv(client_socket, response, sizeof(st_response), 0);
    int port;
    char *ipaddress = (char *)malloc(16 * sizeof(char));
    if (bytes_received == -1)
    {
        perror("Receive failed");
    }
    else
    {
        if (response->response_type == FILE_NOT_FOUND)
        {
            printf(RED_COLOR "File Not Found \n" RESET_COLOR); // Error Not Found File
            // return;
        }
        else if (response->response_type == INVALID_FILETYPE)
        {
            printf(RED_COLOR "Invalid file type\n" RESET_COLOR);
            // return;
        }
        else
        {

            strcpy(ipaddress, response->IP_Addr);
            port = response->Port_No;
            printf("%s %d\n", ipaddress, port);
        }
    }
    close(client_socket);

    if (response->response_type == 200)
    {
        communicate_with_ss(ipaddress, port, path);
    }

    return;
}
