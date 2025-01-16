#include "functions.h"
#define SEND_DELAY_USEC 10000
int stream_audio(st_request *req)
{
    printf("Streaming audio at handler function\n");
    int client_sock=req->socket;
    char full_path[2*MAX_PATH_LEN];
    memset(full_path, 0, sizeof(full_path));
    strcpy(full_path, "main/");
    strcat(full_path, req->src_path);
    // Construct the full path
    // if (strcmp(req->src_path, "/") == 0) {
    //     // snprintf(full_path, sizeof(full_path), "main/%s", req->src_file_dir_name);
    //     strcpy(full_path, "main/");
    //     strcat(full_path, req->src_file_dir_name);
    // } else {
    //     // snprintf(full_path, sizeof(full_path), "main/%s/%s", req->src_path, req->src_file_dir_name);
    //     strcpy(full_path, "main/");
    //     strcat(full_path, req->src_path);
    //     strcat(full_path, "/");
    //     strcat(full_path, req->src_file_dir_name);
    // }
    printf("Full path: %s\n", full_path);
    FILE *file = fopen(full_path, "rb");
    if (file == NULL)
    {
        perror("File not found or cannot be opened");
        close(client_sock);
        return -1;
    }

    char buffer[BUFFER_SIZE*100];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        if (send(client_sock, buffer, bytes_read, 0) < 0)
        {
            perror("Failed to send file data");
            fclose(file);
            close(client_sock);
            return -1;
            // break;
        }
        usleep(SEND_DELAY_USEC); // Add a delay between sends
    }

    fclose(file);
    close(client_sock);
    return 0;
}