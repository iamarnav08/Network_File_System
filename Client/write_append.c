#include "headers.h"
#define LIMIT 10240 // Define buffer limit for input data

typedef struct
{
    int socket;
} thread_arg_t;

// Thread handler to listen for acknowledgment
void *thread_handler(void *arg)
{
    thread_arg_t *t_arg = (thread_arg_t *)arg;
    int client_socket = t_arg->socket;
    free(t_arg); // Free allocated memory for thread arguments

    st_request response;
    while (1)
    {
        ssize_t bytes_received = recv(client_socket, &response, sizeof(st_request), 0);
        if (bytes_received <= 0)
        {
            if (bytes_received == 0)
            {
                printf(RED_COLOR "Connection closed by server.\n" RESET_COLOR);
            }
            else
            {
                perror("Error while receiving acknowledgment");
            }
            break; // Exit the loop on error or connection closure
        }

        if (response.request_type == ACK)
        {
            printf(GREEN_COLOR "Acknowledgment received: %s\n" RESET_COLOR, response.data);
            break;
        }
        else
        {
            printf(YELLOW_COLOR "Unexpected response: %s\n" RESET_COLOR, response.data);
        }
    }

    close(client_socket); // Close the socket in the thread
    pthread_exit(NULL);   // Exit the thread
}

#define CHUNK_SIZE 256 // Define the size of each data chunk

void communicate_with_ss_write(char *ipaddress, int port, char *path, int f)
{
    int client_socket = connect_with_ss(ipaddress, port);
    if (client_socket < 0)
    {
        perror("Connection failed");
        return;
    }

    st_request packet;
    packet.request_type = WRITE_REQ;
    strcpy(packet.src_path, path);
    packet.sync = f;

    if (send(client_socket, &packet, sizeof(st_request), 0) < 0)
    {
        perror("Send failed");
        close(client_socket);
        return;
    }

    printf(BLUE_COLOR "ENTER INPUT TO BE WRITTEN:\n" RESET_COLOR);

    // Read all input data from stdin and store in a buffer
    char *buffer = malloc(LIMIT); // Allocate a buffer for storing data
    if (!buffer)
    {
        perror("Memory allocation failed");
        close(client_socket);
        return;
    }

    int total_data = 0;
    char input;
    while ((input = fgetc(stdin)) != '\n' && input != EOF)
    {
        if (total_data >= LIMIT - 1) // Ensure no buffer overflow
        {
            printf(RED_COLOR "Input exceeds buffer limit.\n" RESET_COLOR);
            break;
        }
        buffer[total_data++] = input;
    }
    buffer[total_data] = '\0'; // Null-terminate the buffer

    // Calculate the number of chunks
    int num_chunks = (total_data + CHUNK_SIZE - 1) / CHUNK_SIZE;
    // return;

    // Send the number of chunks to the server
    if (send(client_socket, &num_chunks, sizeof(num_chunks), 0) == -1)
    {
        perror("Failed to send number of chunks");
        free(buffer);
        close(client_socket);
        return;
    }
    char data[CHUNK_SIZE];
    // Send each chunk sequentially
    for (int i = 0; i < num_chunks; i++)
    {
        memset(data, 0, sizeof(data)); // Clear the packet data buffer
        int chunk_size = (i == num_chunks - 1) ? (total_data % CHUNK_SIZE) : CHUNK_SIZE;
        memcpy(data, buffer + (i * CHUNK_SIZE), chunk_size); // Copy chunk to packet

        // Print the actual chunk size being sent
        printf("Chunk size: %d\n", chunk_size);
        printf("%s\n", data);
        printf("%d\n", client_socket);
        // Send the packet with the correct size
        if (send(client_socket, data, chunk_size, 0) == -1)
        {
            perror("Send failed");
            free(buffer);
            close(client_socket);
            return;
        }
        usleep(100000);
        printf(GREEN_COLOR "Chunk %d sent successfully.\n" RESET_COLOR, i + 1);
    }

    free(buffer); // Free the allocated buffer

    // Receive acknowledgment
    st_request response;
    ssize_t bytes_received = recv(client_socket, &response, sizeof(st_request), 0);
    if (bytes_received <= 0)
    {
        perror("Receiving acknowledgment failed");
    }
    else if (response.request_type == ACK)
    {
        printf(GREEN_COLOR "Write is synchronous.\n" RESET_COLOR);
    }
    else if (response.request_type==ACK_ASYNC)
    {
      printf("Write is aynchronous\n");
    }
    else
    {
        printf(RED_COLOR "Failed to Write Data: %s\n" RESET_COLOR, response.data);
    }

    if (f == 1)
    {
        close(client_socket);
        return;
    }

    // Create a thread to listen for acknowledgment in sync mode
    pthread_t ack_thread;
    thread_arg_t *t_arg = malloc(sizeof(thread_arg_t));
    if (t_arg == NULL)
    {
        perror("Memory allocation for thread arguments failed");
        close(client_socket);
        return;
    }
    t_arg->socket = client_socket;
    if (pthread_create(&ack_thread, NULL, thread_handler, t_arg) != 0)
    {
        perror("Failed to create acknowledgment thread");
        free(t_arg);
        close(client_socket);
        return;
    }

    pthread_detach(ack_thread); // Detach the thread
}

void writing_append_operation(char *path, int f)
{
    // ack* client = connect_with_ns_write();
    int client_socket = connect_with_ns();
    st_request writerpacket;
    writerpacket.request_type = WRITE_REQ;
    writerpacket.sync = f;
    strcpy(writerpacket.src_path, path);
    // strcpy(client->path,path);

    int bytes_sent = send(client_socket, &writerpacket, sizeof(st_request), 0);
    if (bytes_sent == -1)
    {
        perror("Send failed");
    }
    st_response response;
    int bytes_received = recv(client_socket, &response, sizeof(st_response), 0);
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
            printf(RED_COLOR "File Not Found \n" RESET_COLOR); // Error Not Found File
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
            strcpy(ipaddress, response.IP_Addr);
            port = response.Port_No;
        }
    }
    close(client_socket);

    communicate_with_ss_write(ipaddress, port, path, f);
    // printf(GREEN_COLOR"------Write/Append Completed------\n"RESET_COLOR);
    return;
}
