#include "headers.h"

void copy_operation(int req_type,char *src, char *dest)
{
    if(strstr(dest,".txt")!=NULL)
    {
        printf(RED_COLOR"File cannot be a destination path \n"RESET_COLOR);
        return ;
    }
    int client_socket = connect_with_ns();
    st_request  packet;
    packet.request_type = req_type;
    strcpy(packet.src_path,src);
    strcpy(packet.dest_path,dest);
    // printf("%s %s\n", path1, path2);
    int bytes_sent = send(client_socket, &packet, sizeof(st_request), 0);
    if (bytes_sent == -1)
    {
        perror("Send failed");
    }
    st_request response ;
    int bytes_received = recv(client_socket, &response, sizeof(st_request), 0);
    printf("%d\n",response.request_type);
    if (response.request_type == ACK)         
    {
        printf(GREEN_COLOR "Copy of Directory or File succesfull \n" RESET_COLOR);
    }
    else
    {
        printf(RED_COLOR"Copy of Directory or File not succesfull : %s \n"RESET_COLOR,response.data);
    }
    close(client_socket);
}