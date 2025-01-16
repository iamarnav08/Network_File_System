#include "functions.h"
#define LIMIT 20

// Get a PathLock by path
PathLock* get_path_lock(PathLockTable *table, const char *path) {
    pthread_mutex_lock(&table->mutex);

    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->paths[i].path, path) == 0) {
            pthread_mutex_unlock(&table->mutex);
            return &table->paths[i];
        }
    }

    pthread_mutex_unlock(&table->mutex);
    return NULL; // Path not found
}
// Lock a path for writing
int lock_path(PathLock *path_lock) {
    if (sem_wait(&path_lock->lock) == 0) {
        return 0; // Lock acquired
    }
    return -1; // Failed to lock
}

// Unlock a path after writing
void unlock_path(PathLock *path_lock) {
    sem_post(&path_lock->lock);
}

int write_file(st_request *req)
{
        // Open the file for writing
 // Acquire the lock for the file
    printf("Here ....................");
    PathLock *file_lock = get_path_lock(&lock_table, req->src_path);
    if (!file_lock) {
        fprintf(stderr, "Lock not found for path: %s\n", req->src_path);
        st_response error;
        error.response_type = FILE_WRITE_ERROR;
        if (send(req->socket, &error, sizeof(st_response), 0) < 0) {
            perror("Failed to send error response to client");
        } else {
            printf("Error response sent to client\n");
        }
        return -1;
    }

    // Try to acquire the lock without waiting
    if (sem_trywait(&file_lock->lock) != 0) {
        fprintf(stderr, "File is currently being written to: %s\n", req->src_path);
        st_response error;
        error.response_type = FILE_WRITE_ERROR;
        if (send(req->socket, &error, sizeof(st_response), 0) < 0) {
            perror("Failed to send error response to client");
        } else {
            printf("Error response sent to client\n");
        }
        return -1; // Decline the client
    }
    st_response message;
    message.response_type = FILE_FOUND;
    if (send(req->socket, &message, sizeof(st_response), 0) < 0) {
        perror("Failed to send error response to client");
    } else {
        printf("Error response sent to client\n");
    }

    int file_fd;
    ssize_t bytes_written;
    char full_path[2 * MAX_PATH_LEN];
    memset(full_path, 0, sizeof(full_path));

    printf("Source path: %s\n", req->src_path);
    printf("Source file/dir name: %s\n", req->src_file_dir_name);
    snprintf(full_path, sizeof(full_path), "main/%s", req->src_path);

    printf("Full path: %s\n", full_path);

    // Receive the number of chunks from the client
    long num_chunks;
    ssize_t bytes_received = recv(req->socket, &num_chunks, sizeof(num_chunks), 0);
    long total=num_chunks;
    num_chunks = (num_chunks+CHUNK_SIZE-1)/CHUNK_SIZE;
    // printf("Chunks %ld\n", num_chunks);
    if (bytes_received <= 0)
    {
        perror("Failed to receive number of chunks from client");
        // Release the lock
        unlock_path(file_lock);
        return -1;
    }
    printf("Number of chunks to receive: %ld\n", num_chunks);

    // Allocate a buffer to store the received data
    char *buffer = malloc(num_chunks * CHUNK_SIZE);
    if (!buffer)
    {
        perror("Memory allocation for buffer failed");
        // Release the lock
        unlock_path(file_lock);
        return -1;
    }

    size_t total_data = 0; // Keep track of total data received
    char data[CHUNK_SIZE];
    for (int i = 0; i < num_chunks; i++)
    {
        memset(data, 0, sizeof(data));
        // printf("%d\n", req->socket);

        // Calculate the correct chunk size for the last chunk
        int chunk_size = (i == num_chunks - 1) ? (total% CHUNK_SIZE) : CHUNK_SIZE;
        if (chunk_size == 0 && i == num_chunks - 1)
        {
            chunk_size = CHUNK_SIZE; // Handle the case where the last chunk is exactly CHUNK_SIZE
        }

        bytes_received = recv(req->socket, data, chunk_size, MSG_WAITALL);
        if (bytes_received <= 0)
        {
            perror("Failed to receive data chunk from client");
            free(buffer);
            return -1;
        }

        // Copy the received data to the buffer
        memcpy(buffer + total_data, data, bytes_received);
        total_data += bytes_received;

        // printf("Received chunk %d %zd\n", i + 1, bytes_received);
    }

    // Send an acknowledgment if the data length exceeds LIMIT and sync is not required
    if (total_data > LIMIT && req->sync != 1)
    {
        st_request ack;
        ack.request_type = ACK_ASYNC;
        if (send(req->socket, &ack, sizeof(st_request), 0) < 0)
        {
            perror("Failed to send ACK to client");
        }
        else
        {
            printf("ACK sent to client.\n");
        }
    }

    file_fd = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd < 0)
    {
        perror("Failed to open file for writing");
        free(buffer);

        st_request error;
        error.request_type = FILE_WRITE_ERROR;
        if (send(req->socket, &error, sizeof(st_request), 0) < 0)
        {
            perror("Failed to send error response to client");
        }
        else
        {
            printf("Error response sent to client\n");
        }
        // Release the lock
        unlock_path(file_lock);
        return -1;
    }

    printf("Writing to file: %s\n", req->src_path);

    // Write the complete buffer to the file
    printf("Buffer size: %ld\n", strlen(buffer));
    printf("Total data: %ld\n", total_data);
    bytes_written = write(file_fd, buffer, total_data);
    if (bytes_written < 0)
    {
        perror("Failed to write to file");
        close(file_fd);
        free(buffer);

        st_response error;
        error.response_type = FILE_WRITE_ERROR;
        if (send(req->socket, &error, sizeof(st_response), 0) < 0)
        {
            perror("Failed to send error response to client");
        }
        else
        {
            printf("Error response sent to client\n");
        }
        // Release the lock
        unlock_path(file_lock);
        return -1;
    }

    // Ensure all data has been written
    if ((size_t)bytes_written < total_data)
    {
        printf("Warning: Only %zd out of %zu bytes were written.\n", bytes_written, total_data);
    }
    else
    {
        printf("File written successfully: %s\n", req->src_path);
    }

    // Close the file and free the buffer
    close(file_fd);
    // Release the lock
    unlock_path(file_lock);
    
    free(buffer);

    // Send acknowledgment to the client
    // sleep(10);
    st_request ack;
    ack.request_type = ACK_MSG;
    strcpy(ack.data, "File written successfully");
    if (send(req->socket, &ack, sizeof(st_request), 0) < 0)
    {
        perror("Failed to send ACK to client");
    }
    else
    {
        printf("ACK sent to client.\n");
    }

    return 0;
}