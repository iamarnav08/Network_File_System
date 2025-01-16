#include "headers.h"

void create_operation(char *path, char *name,int macro)
{
    int client_socket = connect_with_ns();
    st_request packet;
    packet.request_type = macro;
    strcpy(packet.src_path,path);
    strcpy(packet.src_file_dir_name,name);
    printf("%s %s\n",packet.src_path,packet.src_file_dir_name);
    // snprintf(packet->data, sizeof(packet->data), "%s|%s", path, name);
    int bytes_sent;
    // if (bytes_sent == -1)
    if((bytes_sent = send(client_socket, &packet, sizeof(st_request), 0))<0)
    {
        perror("Send failed");
    }
    st_response response;
    int bytes_received = recv(client_socket, &response, sizeof(st_response), 0);
    if (response.response_type == ACK)
    {
        printf(GREEN_COLOR"Creation of Directory or File succesfull \n"RESET_COLOR);
    }
    else
    {   
        printf(RED_COLOR"Creation of Directory or File not succesfull : %s \n" RESET_COLOR,response.message); // Error Not succesfull
    }
    close(client_socket);
}