#include"headers.h"

void backup_dir_1(int i)
{
    printf("Backing up storage server %d in folder %d in storage server %d\n", i, storage_server_list[i]->backupfoldernumber1, storage_server_list[i]->backup_storage_server1);
    request s_req = (request)malloc(sizeof(st_request));
    request des_r = (request)malloc(sizeof(st_request));
    des_r->request_type = BACKUP_RECEIVE;
    strcpy(des_r->src_path, "main");
    // strcpy(des_r->dest_path, s_req->dest_path);
    strcpy(des_r->ip_for_copy, storage_server_list[storage_server_list[i]->backup_storage_server1]->IP_Addr);
    if(storage_server_list[i]->backupfoldernumber1 == 1)
    {
        strcpy(des_r->dest_path, "b1");
    }
    else if(storage_server_list[i]->backupfoldernumber1 == 2)
    {
        strcpy(des_r->dest_path, "b2");
    }
    else
    {
        strcpy(des_r->dest_path, "b3");
    }
    // strcpy(des_r->src_file_dir_name, "main");
    des_r->port_for_copy = storage_server_list[storage_server_list[i]->backup_storage_server1]->Port_No;
    strcpy(des_r->src_file_dir_name, "main");
    des_r->socket = storage_server_list[i]->storage_server_socket;
    printf("destination path%s\n", des_r->dest_path);
    send(storage_server_list[storage_server_list[i]->backup_storage_server1]->storage_server_socket, des_r, sizeof(st_request), 0);


    s_req->request_type = BACKUP_SEND;
    strcpy(s_req->src_path, "main");   
    strcpy(s_req->ip_for_copy, storage_server_list[storage_server_list[i]->backup_storage_server1]->IP_Addr);
    s_req->port_for_copy = storage_server_list[storage_server_list[i]->backup_storage_server1]->Port_No;
    strcpy(s_req->src_file_dir_name, "main");
    send(storage_server_list[i]->storage_server_socket, s_req, sizeof(st_request), 0);

    

    // response res = (response)malloc(sizeof(st_response));
    // int r = recv(storage_server_list[i]->storage_server_socket, res, sizeof(st_response), 0);
    // if(res->response_type == ACK)
    // {
    //     printf("Files copied successfully\n");
    // }
    // else
    // {
    //     printf("Error in backing up\n");
    // }
}

void backup_dir_2(int i)
{
    printf("Backing up storage server %d in folder %d and storage server %d\n", i, storage_server_list[i]->backupfoldernumber2, storage_server_list[i]->backup_storage_server2);
    request s_req = (request)malloc(sizeof(st_request));
    request des_r = (request)malloc(sizeof(st_request));

    des_r->request_type = BACKUP_RECEIVE;
    strcpy(des_r->src_path, "main");
    strcpy(des_r->ip_for_copy, storage_server_list[storage_server_list[i]->backup_storage_server2]->IP_Addr);
    if(storage_server_list[i]->backupfoldernumber2 == 1)
    {
        strcpy(des_r->dest_path, "b1");
    }
    else if(storage_server_list[i]->backupfoldernumber2 == 2)
    {
        strcpy(des_r->dest_path, "b2");
    }
    else 
    {
        strcpy(des_r->dest_path, "b3");
    }
    // strcpy(des_r->src_file_dir_name, "main");
    strcpy(des_r->ip_for_copy, storage_server_list[storage_server_list[i]->backup_storage_server2]->IP_Addr);
    des_r->port_for_copy = storage_server_list[storage_server_list[i]->backup_storage_server2]->Port_No;
    strcpy(des_r->src_file_dir_name, "main");
    des_r->socket = storage_server_list[i]->storage_server_socket;
    printf("destination path%s\n", des_r->dest_path);
    send(storage_server_list[storage_server_list[i]->backup_storage_server2]->storage_server_socket, des_r, sizeof(st_request), 0);


    s_req->request_type = BACKUP_SEND;
    strcpy(s_req->src_path, "main");
    strcpy(s_req->ip_for_copy, storage_server_list[storage_server_list[i]->backup_storage_server2]->IP_Addr);
    s_req->port_for_copy = storage_server_list[storage_server_list[i]->backup_storage_server2]->Port_No;
    strcpy(s_req->src_file_dir_name, "main");
    s_req->socket = storage_server_list[i]->storage_server_socket;
    send(storage_server_list[i]->storage_server_socket, s_req, sizeof(st_request), 0);


    
    // response res = (response)malloc(sizeof(st_response));
    // int r = recv(storage_server_list[i]->storage_server_socket, res, sizeof(st_response), 0);
    // if(res->response_type == ACK)
    // {
    //     printf("Files copied successfully\n");
    // }
    // else
    // {
    //     printf("Error in backing up\n");
    // }
}

void* backup_handler(){
    int backup_count = 0;
   while(1)
   {
        if(storage_server_count >= 3)
        {
            for(int i=0;i<storage_server_count;i++)
            {
                if(storage_server_list[i]->is_backed_up == false)
                {
                    printf("Backing up storage number: %d\n", backup_count);
                    if(i == 0)
                    {
                        storage_server_list[i]->backup_storage_server1 = 1;
                        storage_server_list[i]->backupfoldernumber1 = 1;
                        storage_server_list[i]->backup_storage_server2 = 2;
                        storage_server_list[i]->backupfoldernumber2 = 1;
                        storage_server_list[i]->is_backed_up = true;
                    }
                    else if(i == 1)
                    {
                        storage_server_list[i]->backup_storage_server1 = 0;
                        storage_server_list[i]->backupfoldernumber1 = 1;
                        storage_server_list[i]->backup_storage_server2 = 2;
                        storage_server_list[i]->backupfoldernumber2 = 2;
                        storage_server_list[i]->is_backed_up = true;
                    }
                    else if(i == 2)
                    {
                        storage_server_list[i]->backup_storage_server1 = 1;
                        storage_server_list[i]->backupfoldernumber1 = 2;
                        storage_server_list[i]->backup_storage_server2 = 0;
                        storage_server_list[i]->backupfoldernumber2 = 2;
                        storage_server_list[i]->is_backed_up = true;
                    }
                    else if(i==3)
                    {
                        storage_server_list[i]->backup_storage_server1 = 0;
                        storage_server_list[i]->backupfoldernumber1 = 3;
                        storage_server_list[i]->backup_storage_server2 = 1;
                        storage_server_list[i]->backupfoldernumber2 = 3;
                        storage_server_list[i]->is_backed_up = true;
                    }
                    else if(i==4)
                    {
                        storage_server_list[i]->backup_storage_server1 = 2;
                        storage_server_list[i]->backupfoldernumber1 = 3;
                        storage_server_list[i]->backup_storage_server2 = 3;
                        storage_server_list[i]->backupfoldernumber2 = 1;
                        storage_server_list[i]->is_backed_up = true;
                    }
                    else
                    {
                        storage_server_list[i]->backup_storage_server1 = (i-1)%storage_server_count;
                        storage_server_list[i]->backupfoldernumber1 = 1;
                        storage_server_list[i]->backup_storage_server2 = (i-2)%storage_server_count;
                        storage_server_list[i]->backupfoldernumber2 = 2;
                        storage_server_list[i]->is_backed_up = true;
                    }
                    backup_dir_1(i); // i is the index of the storage server which is to be backed up
                    usleep(1000000);
                    backup_dir_2(i);
                    usleep(1000000);
                    backup_count++;
                    // return NULL;
                } 
            }
        }
   }
}