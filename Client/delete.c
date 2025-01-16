#include "headers.h"

void delete_operation(char *path,char * name ,int macro)
{
    int client_socket = connect_with_ns();
    st_request packet;
    packet.request_type = macro;
    strcpy(packet.src_path,path);
    strcpy(packet.src_file_dir_name,name);
    // snprintf(packet.data, sizeof(packet->data), "%s", path);
    int bytes_sent = send(client_socket, &packet, sizeof(st_request), 0);
    if (bytes_sent == -1)
    {
        perror("Send failed");
    }
    st_response response ;
    int bytes_received = recv(client_socket, &response, sizeof(st_response), 0);
    if (response.response_type==ACK)
    {
        printf(GREEN_COLOR"Deletion of Directory or File succesfull \n"RESET_COLOR);
    }
    else
    {
        printf(RED_COLOR"Deletion of Directory or File not succesfull : %s\n"RESET_COLOR, response.message); // Error Not succesfull
    }
    close(client_socket);
}