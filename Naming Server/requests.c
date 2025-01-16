#include "headers.h"
extern node* hashtable; 

// only arrive here if the request is a READ, WRITE, GET_FILE_INFO or STREAM_AUDIO
void handle_file_request(request req, int client_id){
    if(req->request_type == READ_FILE || req->request_type == WRITE_FILE){
        if(strstr(req->src_path, ".txt") == NULL){
            // printf("Here\n");
            response r = (response)malloc(sizeof(st_response));
            r->response_type = INVALID_FILETYPE;
            strcpy(r->message, "Incompatible file opened!");
            strcpy(r->IP_Addr, "");
            r->Port_No = -1;
            send(client_id, r, sizeof(st_request), 0);
            add_to_cache(req, r, -1);
            logMessage(CLIENT_FLAG, client_id, *req, INVALID_FILETYPE,0);
            return;
        }
    }
    if(req->request_type == STREAM_AUDIO) {
        if(strstr(req->src_path, ".mp3") == NULL){
            response r = (response)malloc(sizeof(st_response));
            r->response_type = INVALID_FILETYPE;
            strcpy(r->message, "Incompatible file opened!");
            strcpy(r->IP_Addr, "");
            r->Port_No = -1;
            send(client_id, r, sizeof(st_request), 0);
            add_to_cache(req, r, -1);
            logMessage(CLIENT_FLAG, client_id, *req, INVALID_FILETYPE,0);
            return;
        }
    } 
    response r = (response)malloc(sizeof(st_response));
    printf("%s\n", req->src_path);
    int id = Get(req->src_path);
    printf("id: %d\n", id);
    int folder = 0;
    if(id!=-1) {
        // snprintf(r->message, sizeof(r->message), "%s | %d", storage_server_list[id]->IP_Addr, storage_server_list[id]->Port_No);
        if(storage_server_list[id]->is_active == false){
            if(req->request_type == WRITE_FILE || req->request_type == STREAM_AUDIO || req->request_type == GET_FILE_INFO) 
            {
                r->response_type = FILE_NOT_FOUND;
                strcpy(r->message, "Storage server is down, you can only READ the file");
                strcpy(r->IP_Addr, "");
                r->Port_No = -1;
                send(client_id, r, sizeof(st_response), 0);
                add_to_cache(req, r, -1);
                logMessage(CLIENT_FLAG, client_id, *req, FILE_NOT_FOUND,0);
                return;
            }
            else if(storage_server_list[storage_server_list[id]->backup_storage_server1]->is_active == true || storage_server_list[storage_server_list[id]->backup_storage_server2]->is_active == true){
                if(storage_server_list[storage_server_list[id]->backup_storage_server1]->is_active == true){
                    id = storage_server_list[id]->backup_storage_server1;
                    folder = storage_server_list[id]->backupfoldernumber1;
                } else {
                    id = storage_server_list[id]->backup_storage_server2;
                    folder = storage_server_list[id]->backupfoldernumber2;
                }
            } else {
                printf("Backup servers are also down\n");
                r->response_type = FILE_NOT_FOUND;
                strcpy(r->message, "File not found");
                strcpy(r->IP_Addr, "");
                r->Port_No = -1;
                send(client_id, r, sizeof(st_response), 0);
                add_to_cache(req, r, -1);
                logMessage(CLIENT_FLAG, client_id, *req, FILE_NOT_FOUND,0);
                return;
            }
        }
        r->response_type = FILE_FOUND;
        strcpy(r->IP_Addr, storage_server_list[id]->IP_Addr);
        // message(backup) | folder
        snprintf(r->message, sizeof(r->message), "Backup | %d", folder);   
        r->Port_No = storage_server_list[id]->Client_Port;
        send(client_id, r, sizeof(st_response), 0);
        add_to_cache(req, r, id);
        logMessage(CLIENT_FLAG, client_id, *req, FILE_FOUND,0);
    } else {
        r->response_type = FILE_NOT_FOUND;
        strcpy(r->message, "File not found");
        strcpy(r->IP_Addr, "");
        r->Port_No = -1;
        send(client_id, r, sizeof(st_response), 0);
        add_to_cache(req, r, -1);
        logMessage(CLIENT_FLAG, client_id, *req, FILE_NOT_FOUND,0);
    }
    free(r);
}


// only arrive here if the request is a COPY, CREATE or DELETE
void file_requests_to_storage_server(request req, int client_id)
{
    int storage_server_id = -1;
    if((req->request_type == DELETE_FILE || req->request_type == DELETE_DIR) && strcmp(req->src_path, "/") == 0)
    {
        storage_server_id = Get(req->src_file_dir_name);
        printf("Deleting root directory or file\n");
    }
    else
        storage_server_id = Get(req->src_path);

    request r = (request)malloc(sizeof(st_request));  // send it to the main source storage server
    if(storage_server_id == -1)
    {
        response res = (response)malloc(sizeof(st_response));
        res->response_type = PATH_NOT_FOUND;
        strcpy(res->message, "File not found");
        strcpy(res->IP_Addr, "");
        res->Port_No = -1;
        send(client_id, res, sizeof(st_response), 0);
        logMessage(CLIENT_FLAG, client_id, *req, PATH_NOT_FOUND,0);
        free(r);
        return;
    }
    else if(req->request_type == CREATE_FILE)
    {
        if(storage_server_list[storage_server_id]->is_active == false)
        {
            response res = (response)malloc(sizeof(st_response));
            res->response_type = PATH_NOT_FOUND;
            strcpy(res->message, "Storage server is down, cannot create file");
            strcpy(res->IP_Addr, "");
            res->Port_No = -1;
            send(client_id, res, sizeof(st_response), 0);
            logMessage(CLIENT_FLAG, client_id, *req, PATH_NOT_FOUND,0);
            free(r);
            return;
        }
        // send a request to the storage server
        char src_full_path[100];
        response res = (response)malloc(sizeof(st_response));
        r->request_type = CREATE_FILE;
        strcpy(r->src_file_dir_name, req->src_file_dir_name);
        snprintf(src_full_path, sizeof(src_full_path), "main/%s", req->src_path);
        strcpy(r->data, req->data);  
        strcpy(r->src_path, src_full_path);
        printf("Creating file\n");
        send(storage_server_list[storage_server_id]->storage_server_socket, r, sizeof(st_request), 0);

        // receive the response from the storage server and send it to the client
        int r1 = recv(socket_arr[storage_server_id][0], res, sizeof(st_response), 0);
        if(res->response_type == ACK)
        {
            printf("File %s created successfully\n", req->src_file_dir_name);
            // insert the path into the hash table 
            char s[100];
            snprintf(s, sizeof(s), "%s/%s", req->src_path, req->src_file_dir_name);
            printf("Inserted path: %s in the hash table\n", s);
            if(Get(s) == -1)
                Insert(s, storage_server_id);
            send(client_id, res, sizeof(st_response), 0);

            // now as the file has got created so also create it in both the backups of that storage server

            // create the file in the backup storage server 1
            // change the variables to avoid errors
            char backup_full_path[MAX_DATA_LENGTH];
            if(storage_server_list[storage_server_id]->is_backed_up == true)
            {
                request backup_req = (request)malloc(sizeof(st_request));
                if(storage_server_list[storage_server_id]->backup_storage_server1 != -1)
                {
                    int index = storage_server_list[storage_server_id]->backup_storage_server1;
                    if(storage_server_list[index]->backupfoldernumber1 == 1)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b1/%s",req->src_path);
                    }
                    else if(storage_server_list[index]->backup_storage_server1 == 2)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b2/%s",req->src_path);
                    }
                    else
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b3/%s",req->src_path);
                    }
                    
                    backup_req->request_type = CREATE_FILE;
                    printf("Creating file in backup storage server 1 %s\n", backup_full_path);
                    strcpy(backup_req->src_file_dir_name, req->src_file_dir_name);
                    strcpy(backup_req->src_path, backup_full_path);
                    strcpy(backup_req->data, req->data);
                    send(storage_server_list[storage_server_id]->storage_server_socket, backup_req, sizeof(st_request), 0);

                    response b1 = (response)malloc(sizeof(st_response));
                    int r1 = recv(storage_server_list[storage_server_id]->storage_server_socket, b1, sizeof(st_response), 0);
                    if(b1->response_type == ACK)
                    {
                        printf("File %s created successfully in backup storage server 1\n", req->src_file_dir_name);
                    }
                    else
                    {
                        printf("Error in creating file in backup storage server 1\n");
                    }
                }
                if(storage_server_list[storage_server_id]->backup_storage_server2 != -1)
                {
                    int index = storage_server_list[storage_server_id]->backup_storage_server2;
                    if(storage_server_list[index]->backupfoldernumber2 == 1)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b1/%s",req->src_path);
                    }
                    else if(storage_server_list[index]->backup_storage_server2 == 2)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b2/%s",req->src_path);
                    }
                    else
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b3/%s",req->src_path);
                    }
                    
                    backup_req->request_type = CREATE_FILE;
                    printf("Creating file in backup storage server 2 %s\n", backup_full_path);
                    strcpy(backup_req->src_file_dir_name, req->src_file_dir_name);
                    strcpy(backup_req->src_path, backup_full_path);
                    strcpy(backup_req->data, req->data);
                    send(storage_server_list[storage_server_id]->storage_server_socket, backup_req, sizeof(st_request), 0);

                    response b2 = (response)malloc(sizeof(st_response));
                    int r1 = recv(storage_server_list[storage_server_id]->storage_server_socket, b2, sizeof(st_response), 0);
                    if(b2->response_type == ACK)
                    {
                        printf("File %s created successfully in backup storage server 2\n", req->src_file_dir_name);
                    }
                    else
                    {
                        printf("Error in creating file in backup storage server 2\n");
                    }
                }
            }
        }
        else
        {
            response res1 = (response)malloc(sizeof(st_response));
            res1->response_type = FILE_CREATE_ERROR;
            strcpy(res1->message, "File already exists");
            strcpy(res1->IP_Addr, "");
            res1->Port_No = -1;
            send(client_id, res1, sizeof(st_response), 0);
        }
        logMessage(CLIENT_FLAG, client_id, *req, PATH_FOUND,0);
        // ss respose left
    }
    else if(req->request_type == CREATE_DIR)
    {
        if(storage_server_list[storage_server_id]->is_active == false)
        {
            response res = (response)malloc(sizeof(st_response));
            res->response_type = PATH_NOT_FOUND;
            strcpy(res->message, "Storage server is down, cannot create directory");
            strcpy(res->IP_Addr, "");
            res->Port_No = -1;
            send(client_id, res, sizeof(st_response), 0);
            logMessage(CLIENT_FLAG, client_id, *req, PATH_NOT_FOUND,0);
            free(r);
            return;
        }
        char backup_full_path[MAX_DATA_LENGTH];
        printf("Creating directory\n");
        response res = (response)malloc(sizeof(st_response));
        r->request_type = CREATE_DIR;
        strcpy(r->src_file_dir_name, req->src_file_dir_name);
        snprintf(backup_full_path, sizeof(backup_full_path), "main/%s", req->src_path);
        strcpy(r->src_path, backup_full_path);
        strcpy(r->data,req->data);
        send(storage_server_list[storage_server_id]->storage_server_socket, r, sizeof(st_request), 0);

        // receive the response from the storage server and send it to the client
        int r1 = recv(socket_arr[storage_server_id][0], res, sizeof(st_response), 0);
        printf("Response received %d\n", res->response_type);
        if(res->response_type == ACK)
        {
            char s[100];
            snprintf(s, sizeof(s), "%s/%s", req->src_path, req->src_file_dir_name);
            printf("Directory %s created successfully\n", s);
            if(Get(s) == -1)
                Insert(s, storage_server_id);
            printf("Inserted path: %s in the hash table\n", s);
            send(client_id, res, sizeof(st_response), 0);

            // now as the directory has got created so also create it in both the backups of that storage server

            // create the directory in the backup storage server 1
            // change the variables to avoid errors
            if(storage_server_list[storage_server_id]->is_backed_up == true)
            {
                request backup_req = (request)malloc(sizeof(st_request));
                if(storage_server_list[storage_server_id]->backup_storage_server1 != -1)
                {
                    int index = storage_server_list[storage_server_id]->backup_storage_server1;
                    if(storage_server_list[index]->backupfoldernumber1 == 1)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b1/%s",req->src_path);
                    }
                    else if(storage_server_list[index]->backup_storage_server1 == 2)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b2/%s",req->src_path);
                    }
                    else
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b3/%s",req->src_path);
                    }
                    
                    backup_req->request_type = CREATE_DIR;
                    printf("Creating directory in backup storage server 1 %s\n", backup_full_path);
                    strcpy(backup_req->src_file_dir_name, req->src_file_dir_name);
                    strcpy(backup_req->src_path, backup_full_path);
                    strcpy(backup_req->data, req->data);
                    send(storage_server_list[storage_server_id]->storage_server_socket, backup_req, sizeof(st_request), 0);

                    response b1 = (response)malloc(sizeof(st_response));
                    int r1 = recv(storage_server_list[storage_server_id]->storage_server_socket, b1, sizeof(st_response), 0);
                    if(b1->response_type == ACK)
                    {
                        printf("Directory %s created successfully in backup storage server 1\n", req->src_file_dir_name);
                    }
                    else
                    {
                        printf("Error in creating directory in backup storage server 1\n");
                    }
                }
                if(storage_server_list[storage_server_id]->backup_storage_server2 != -1)
                {
                    int index = storage_server_list[storage_server_id]->backup_storage_server2;
                    if(storage_server_list[index]->backupfoldernumber2 == 1)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b1/%s",req->src_path);
                    }
                    else if(storage_server_list[index]->backup_storage_server2 == 2)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b2/%s",req->src_path);
                    }
                    else
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b3/%s",req->src_path);
                    }
                }
                backup_req->request_type = CREATE_DIR;
                printf("Creating directory in backup storage server 2 %s\n", backup_full_path);
                strcpy(backup_req->src_file_dir_name, req->src_file_dir_name);
                strcpy(backup_req->src_path, backup_full_path);
                strcpy(backup_req->data, req->data);
                send(storage_server_list[storage_server_id]->storage_server_socket, backup_req, sizeof(st_request), 0);

                response b2 = (response)malloc(sizeof(st_response));
                int r1 = recv(storage_server_list[storage_server_id]->storage_server_socket, b2, sizeof(st_response), 0);
                if(b2->response_type == ACK)
                {
                    printf("Directory %s created successfully in backup storage server 2\n", req->src_file_dir_name);
                }
                else
                {
                    printf("Error in creating directory in backup storage server 2\n");
                }
            }
        }
        else
        {
            response res1 = (response)malloc(sizeof(st_response));
            res1->response_type = CREATE_DIR_ERROR;
            strcpy(res1->message, "Directory already exists");
            strcpy(res1->IP_Addr, "");
            res1->Port_No = -1;
            send(client_id, res1, sizeof(st_response), 0);
        }
        logMessage(CLIENT_FLAG, client_id, *req, PATH_FOUND,0);
        // ss response left
    }
    else if(req->request_type == DELETE_FILE)
    {
        int storage_server_id1 = -1;
        char s1[100];
        // concatenate the path with the file name
        if(strcmp(req->src_path, "/") == 0)
        {
            storage_server_id1 = Get(req->src_file_dir_name);
            strcpy(s1, req->src_file_dir_name);
        }
        else
        {    
            snprintf(s1, sizeof(s1), "%s/%s", req->src_path, req->src_file_dir_name);
            storage_server_id1 = Get(s1);
        }
        printf("%s\n", s1);
        if(storage_server_list[storage_server_id1]->is_active == false)
        {
            response res = (response)malloc(sizeof(st_response));
            res->response_type = PATH_NOT_FOUND;
            strcpy(res->message, "Storage server is down, cannot delete file");
            strcpy(res->IP_Addr, "");
            res->Port_No = -1;
            send(client_id, res, sizeof(st_response), 0);
            logMessage(CLIENT_FLAG, client_id, *req, PATH_NOT_FOUND,0);
            free(r);
            return;
        }
        printf("Deleting file\n");
        char src_full_path[100];
        response res = (response)malloc(sizeof(st_response));
        r->request_type = DELETE_FILE;
        strcpy(r->src_file_dir_name, req->src_file_dir_name);
        snprintf(src_full_path, sizeof(src_full_path), "main/%s", req->src_path);
        strcpy(r->src_path, src_full_path);
        strcpy(r->data,req->data);
        send(storage_server_list[storage_server_id1]->storage_server_socket, r, sizeof(st_request), 0);

        // receive the response from the storage server and send it to the client
        int r1 = recv(socket_arr[storage_server_id1][0], res, sizeof(st_response), 0);
        if(res->response_type == ACK)
        {
            printf("File %s deleted successfully\n", s1);
            // delete the path from the hash table
            Delete(s1);
            printf("Deleted path: %s from the hash table", s1);
            send(client_id, res, sizeof(st_response), 0);

            // now as the file has got deleted so also delete it in both the backups of that storage server

            // delete the file in the backup storage server 1
            // change the variables to avoid errors
            char backup_full_path[MAX_DATA_LENGTH];
            if(storage_server_list[storage_server_id1]->is_backed_up == true)
            {
                request backup_req = (request)malloc(sizeof(st_request));
                if(storage_server_list[storage_server_id1]->backup_storage_server1 != -1)
                {
                    int index = storage_server_list[storage_server_id1]->backup_storage_server1;
                    if(storage_server_list[index]->backupfoldernumber1 == 1)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b1/%s",req->src_path);
                    }
                    else if(storage_server_list[index]->backup_storage_server1 == 2)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b2/%s",req->src_path);
                    }
                    else
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b3/%s",req->src_path);
                    }
                    
                    backup_req->request_type = DELETE_FILE;
                    printf("Deleting file in backup storage server 1 %s\n", backup_full_path);
                    strcpy(backup_req->src_file_dir_name, req->src_file_dir_name);
                    strcpy(backup_req->src_path, backup_full_path);
                    strcpy(backup_req->data, req->data);
                    send(storage_server_list[storage_server_id1]->storage_server_socket, backup_req, sizeof(st_request), 0);

                    response b1 = (response)malloc(sizeof(st_response));
                    int r1 = recv(storage_server_list[storage_server_id1]->storage_server_socket, b1, sizeof(st_response), 0);
                    if(b1->response_type == ACK)
                    {
                        printf("File %s deleted successfully in backup storage server 1\n", req->src_file_dir_name);
                    }
                    else
                    {
                        printf("Error in deleting file in backup storage server 1\n");
                    }
                }
                if(storage_server_list[storage_server_id1]->backup_storage_server2 != -1)
                {
                    int index = storage_server_list[storage_server_id1]->backup_storage_server2;
                    if(storage_server_list[index]->backupfoldernumber2 == 1)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b1/%s",req->src_path);
                    }
                    else if(storage_server_list[index]->backup_storage_server2 == 2)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b2/%s",req->src_path);
                    }
                    else
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b3/%s",req->src_path);
                    }
                }
                backup_req->request_type = DELETE_FILE;
                printf("Deleting file in backup storage server 2 %s\n", backup_full_path);
                strcpy(backup_req->src_file_dir_name, req->src_file_dir_name);
                strcpy(backup_req->src_path, backup_full_path);
                strcpy(backup_req->data, req->data);
                send(storage_server_list[storage_server_id1]->storage_server_socket, backup_req, sizeof(st_request), 0);

                response b2 = (response)malloc(sizeof(st_response));
                int r1 = recv(storage_server_list[storage_server_id1]->storage_server_socket, b2, sizeof(st_response), 0);
                if(b2->response_type == ACK)
                {
                    printf("File %s deleted successfully in backup storage server 2\n", req->src_file_dir_name);
                }
                else
                {
                    printf("Error in deleting file in backup storage server 2\n");
                }
            }
        }
        else
        {
            response res1 = (response)malloc(sizeof(st_response));
            res1->response_type = FILE_DELETE_ERROR;
            strcpy(res1->message, "File not found");
            strcpy(res1->IP_Addr, "");
            res1->Port_No = -1;
            send(client_id, res1, sizeof(st_response), 0);
        }
        logMessage(CLIENT_FLAG, client_id, *req, PATH_FOUND,0);
        // ss response left
    }
    else if(req->request_type == DELETE_DIR)
    {
        char s1[100];
        int storage_server_id1 = -1;
        if(strcmp(req->src_path, "/") == 0)
        {
            storage_server_id1 = Get(req->src_file_dir_name);
            strcpy(s1, req->src_file_dir_name);
        }
        else
        {
            snprintf(s1, sizeof(s1), "%s/%s", req->src_path, req->src_file_dir_name);
            storage_server_id1 = Get(req->src_path);
        }
        if(storage_server_list[storage_server_id1]->is_active == false)
        {
            response res = (response)malloc(sizeof(st_response));
            res->response_type = PATH_NOT_FOUND;
            strcpy(res->message, "Storage server is down, cannot delete directory");
            strcpy(res->IP_Addr, "");
            res->Port_No = -1;
            send(client_id, res, sizeof(st_response), 0);
            logMessage(CLIENT_FLAG, client_id, *req, PATH_NOT_FOUND,0);
            free(r);
            return;
        }
        printf("Deleting directory\n");
        char src_full_path[100];
        response res = (response)malloc(sizeof(st_response));
        r->request_type = DELETE_DIR;
        strcpy(r->src_file_dir_name, req->src_file_dir_name);
        snprintf(src_full_path, sizeof(src_full_path), "main/%s", req->src_path);
        strcpy(r->src_path, src_full_path);
        strcpy(r->data,req->data);
        send(storage_server_list[storage_server_id]->storage_server_socket, r, sizeof(st_request), 0);

        // receive the response from the storage server and send it to the client
        int r1 = recv(storage_server_list[storage_server_id1]->storage_server_socket, res, sizeof(st_response), 0);
        printf("Response received %d\n", res->response_type);
        if(res->response_type == ACK)
        {
            printf("Directory %s deleted successfully\n", s1);
            // delete the path from the hash table
            Delete(s1);
            printf("Deleted path: %s from the hash table\n", s1);
            send(client_id, res, sizeof(st_response), 0);

            // now as the directory has got deleted so also delete it in both the backups of that storage server

            // delete the directory in the backup storage server 1
            // change the variables to avoid errors

            char backup_full_path[MAX_DATA_LENGTH];
            if(storage_server_list[storage_server_id1]->is_backed_up == true)
            {
                request backup_req = (request)malloc(sizeof(st_request));
                if(storage_server_list[storage_server_id1]->backup_storage_server1 != -1)
                {
                    int index = storage_server_list[storage_server_id1]->backup_storage_server1;
                    if(storage_server_list[index]->backupfoldernumber1 == 1)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b1/%s",req->src_path);
                    }
                    else if(storage_server_list[index]->backup_storage_server1 == 2)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b2/%s",req->src_path);
                    }
                    else
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b3/%s",req->src_path);
                    }
                    
                    backup_req->request_type = DELETE_DIR;
                    printf("Deleting directory in backup storage server 1 %s\n", backup_full_path);
                    strcpy(backup_req->src_file_dir_name, req->src_file_dir_name);
                    strcpy(backup_req->src_path, backup_full_path);
                    strcpy(backup_req->data, req->data);
                    send(storage_server_list[storage_server_id1]->storage_server_socket, backup_req, sizeof(st_request), 0);

                    response b1 = (response)malloc(sizeof(st_response));
                    int r1 = recv(storage_server_list[storage_server_id1]->storage_server_socket, b1, sizeof(st_response), 0);
                    if(b1->response_type == ACK)
                    {
                        printf("Directory %s deleted successfully in backup storage server 1\n", req->src_file_dir_name);
                    }
                    else
                    {
                        printf("Error in deleting directory in backup storage server 1\n");
                    }
                }
                if(storage_server_list[storage_server_id1]->backup_storage_server2 != -1)
                {
                    int index = storage_server_list[storage_server_id1]->backup_storage_server2;
                    if(storage_server_list[index]->backupfoldernumber2 == 1)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b1/%s",req->src_path);
                    }
                    else if(storage_server_list[index]->backup_storage_server2 == 2)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b2/%s",req->src_path);
                    }
                    else
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b3/%s",req->src_path);
                    }
                }
                backup_req->request_type = DELETE_DIR;
                printf("Deleting directory in backup storage server 2 %s\n", backup_full_path);
                strcpy(backup_req->src_file_dir_name, req->src_file_dir_name);
                strcpy(backup_req->src_path, backup_full_path);
                strcpy(backup_req->data, req->data);
                send(storage_server_list[storage_server_id1]->storage_server_socket, backup_req, sizeof(st_request), 0);

                response b2 = (response)malloc(sizeof(st_response));
                int r1 = recv(storage_server_list[storage_server_id1]->storage_server_socket, b2, sizeof(st_response), 0);
                if(b2->response_type == ACK)
                {
                    printf("Directory %s deleted successfully in backup storage server 2\n", req->src_file_dir_name);
                }
                else
                {
                    printf("Error in deleting directory in backup storage server 2\n");
                }
            }
        }
        else
        {
            response res1 = (response)malloc(sizeof(st_response));
            res1->response_type = DELETE_DIR_ERROR;
            strcpy(res1->message, "Directory not found");
            strcpy(res1->IP_Addr, "");
            res1->Port_No = -1;
            send(client_id, res1, sizeof(st_response), 0);
        }
        logMessage(CLIENT_FLAG, client_id, *req, PATH_FOUND,0);
        // ss response left
    }
    else if(req->request_type == COPY_FILE)
    {
        if(storage_server_list[storage_server_id]->is_active == false)
        {
            response res = (response)malloc(sizeof(st_response));
            res->response_type = PATH_NOT_FOUND;
            strcpy(res->message, "Storage server is down, cannot copy file\n");
            strcpy(res->IP_Addr, "");
            res->Port_No = -1;
            send(client_id, res, sizeof(st_response), 0);
            logMessage(CLIENT_FLAG, client_id, *req, PATH_NOT_FOUND,0);
            free(r);
            return;
        }
        int res4 = Get(req->dest_path);
        request des_r = (request)malloc(sizeof(st_request));
        if(res4 == -1)
        {
            response res = (response)malloc(sizeof(st_response));
            res->response_type = COPY_TO_PATH_INVALID;
            strcpy(res->message, "Path not found");
            strcpy(res->IP_Addr, "");
            res->Port_No = -1;
            send(client_id, res, sizeof(st_response), 0);
            logMessage(CLIENT_FLAG, client_id, *req, COPY_TO_PATH_INVALID,0);
            free(r);
            return;
        } else if(res4 == storage_server_id){

            printf("Copying to the same storage server\n");
            r = (request)malloc(sizeof(st_request));
            r->request_type = COPY_TO_SAME_FILE;
            strcpy(r->src_file_dir_name, req->src_file_dir_name);

            char full_path[100];
            char copy_path[100];
            strcpy(copy_path, req->src_path);
            snprintf(full_path, sizeof(full_path), "main/%s", req->src_path);
            char full_dest_path[100];
            snprintf(full_dest_path, sizeof(full_dest_path), "main/%s", req->dest_path);

            char* token = strtok(req->src_path, "/");
            while(token != NULL)
            {
                strcpy(r->src_file_dir_name, token);
                token = strtok(NULL, "/");
            }
            strcpy(r->src_path, full_path);
            strcpy(r->dest_path, full_dest_path);
            strcpy(r->data, req->data);     
            strcpy(r->ip_for_copy, storage_server_list[res4]->IP_Addr);
            r->port_for_copy = storage_server_list[res4]->Port_No; 
            send(storage_server_list[storage_server_id]->storage_server_socket, r, sizeof(st_request), 0);

            response same_res = (response)malloc(sizeof(st_response));
            int r1 = recv(storage_server_list[storage_server_id]->storage_server_socket, same_res, sizeof(st_response), 0);
            if(same_res->response_type == ACK)
            {
                // file is being copied to the backup storage server, so no need to insert the path in the hash table
                send(client_id, same_res, sizeof(st_response), 0);
                // logMessage(CLIENT_FLAG, client_id, *req, PATH_FOUND,0);
            }
            else
            {
                response res = (response)malloc(sizeof(st_response));
                res->response_type = FILE_COPY_ERROR;
                strcpy(res->message, "Copy failed");
                strcpy(res->IP_Addr, "");
                res->Port_No = -1;
                send(client_id, res, sizeof(st_response), 0);
                // logMessage(CLIENT_FLAG, client_id, *req, FILE_COPY_ERROR,0);
            }
            free(r);
            return;
        }
        r->request_type = COPY_FILE;
        char copy_path[100];
        char full_path[100];
        strcpy(copy_path, req->src_path);
        snprintf(full_path, sizeof(full_path), "main/%s", req->src_path);
        char full_dest_path[100];
        snprintf(full_dest_path, sizeof(full_dest_path), "main/%s", req->dest_path);
        
        // tokenise the copy to get the file name
        
        // get the last token and copy it to the src_file_dir_name

        strcpy(r->src_file_dir_name, req->src_file_dir_name);
        strcpy(r->src_path, full_path);
        strcpy(r->dest_path, full_dest_path);
        strcpy(r->data, req->data);
        strcpy(r->ip_for_copy, storage_server_list[res4]->IP_Addr);
        r->port_for_copy = storage_server_list[res4]->Port_No;
        // printf("r->port_for_copy: %d\n", r->port_for_copy);
        r->socket = storage_server_list[res4]->ss_socket;
        send(storage_server_list[storage_server_id]->storage_server_socket, r, sizeof(st_request), 0);
        
        des_r->request_type = RECIEVE_FILE;

        full_path[100];
        snprintf(full_path, sizeof(full_path), "main/%s", req->src_path);
        full_dest_path[100];
        snprintf(full_dest_path, sizeof(full_dest_path), "main/%s", req->dest_path);

        char *token = strtok(copy_path, "/");
        while(token != NULL)
        {
            strcpy(des_r->src_file_dir_name, token);
            token = strtok(NULL, "/");
        }

        printf("file name %s\n", des_r->src_file_dir_name);
        strcpy(des_r->src_path, full_path);
        strcpy(des_r->dest_path, full_dest_path);
        strcpy(des_r->data, req->data);
        strcpy(des_r->ip_for_copy, storage_server_list[storage_server_id]->IP_Addr);
        des_r->port_for_copy = storage_server_list[storage_server_id]->Port_No;
        des_r->socket = storage_server_list[storage_server_id]->ss_socket;
        printf("Path sent : %s\n", full_dest_path);
        // printf("%d\n", storage_server_list[res]->Port_No);
        send(storage_server_list[res4]->storage_server_socket, des_r, sizeof(st_request), 0);

        response r3 = (response)malloc(sizeof(st_response));
        int r1 = recv(storage_server_list[storage_server_id]->storage_server_socket, r3, sizeof(st_response), 0);
        if(r1 < 0)
        {
            printf("Error in copying file\n");
            response res = (response)malloc(sizeof(st_response));
            res->response_type = FILE_COPY_ERROR;
            strcpy(res->message, "Copy failed");
            strcpy(res->IP_Addr, "");
            res->Port_No = -1;
            send(client_id, res, sizeof(st_response), 0);
            logMessage(CLIENT_FLAG, client_id, *req, FILE_COPY_ERROR,0);
            return;
        }
        if(r3->response_type == ACK)
        {
            // insert the path into the hash table
            char s[100];
            snprintf(s, sizeof(s), "%s/%s", req->dest_path, des_r->src_file_dir_name);
            printf("Inserted path: %s in the hash table", s);
            if(Get(s) == -1)
                Insert(s, res4);
            send(client_id, r3, sizeof(st_response), 0);

            // now as the file has got copied so also copy it in both the backups of that storage server
            char backup_full_path[MAX_DATA_LENGTH];
            if(storage_server_list[storage_server_id]->is_backed_up == true)
            {
                if(storage_server_list[storage_server_id]->backup_storage_server1 != -1)
                {
                    int index = storage_server_list[storage_server_id]->backup_storage_server1;
                    if(storage_server_list[index]->backupfoldernumber1 == 1)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b1/%s",req->dest_path);
                    }
                    else if(storage_server_list[index]->backup_storage_server1 == 2)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b2/%s",req->dest_path);
                    }
                    else
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b3/%s",req->dest_path);
                    }
                    
                    printf("Copying file in backup storage server 1 %s\n", backup_full_path);

                    request backup_r1 = (request)malloc(sizeof(st_request));
                    backup_r1->request_type = COPY_FILE;
                    char copy_path[100];
                    char src_full_path[100];
                    strcpy(copy_path, req->src_path);
                    snprintf(src_full_path, sizeof(src_full_path), "main/%s", req->src_path);

                    strcpy(backup_r1->src_file_dir_name, req->src_file_dir_name);
                    strcpy(backup_r1->src_path, src_full_path);
                    strcpy(backup_r1->dest_path, backup_full_path);
                    strcpy(backup_r1->data, req->data);
                    strcpy(backup_r1->ip_for_copy, storage_server_list[index]->IP_Addr);
                    backup_r1->port_for_copy = storage_server_list[index]->Port_No;
                    backup_r1->socket = storage_server_list[index]->ss_socket;
                    send(storage_server_list[storage_server_id]->storage_server_socket, backup_r1, sizeof(st_request), 0);

                    request backup_r2 = (request)malloc(sizeof(st_request));
                    backup_r2->request_type = RECIEVE_FILE;
                    char dest_full_path[100];
                    snprintf(dest_full_path, sizeof(dest_full_path), "main/%s", req->src_path);
                    
                    char *token = strtok(copy_path, "/");
                    while(token != NULL)
                    {
                        strcpy(backup_r2->src_file_dir_name, token);
                        token = strtok(NULL, "/");
                    }

                    strcpy(backup_r2->src_path, full_path);
                    strcpy(backup_r2->dest_path, dest_full_path);
                    strcpy(backup_r2->data, req->data);
                    strcpy(backup_r2->ip_for_copy, storage_server_list[storage_server_id]->IP_Addr);
                    backup_r2->port_for_copy = storage_server_list[storage_server_id]->Port_No;
                    backup_r2->socket = storage_server_list[storage_server_id]->ss_socket;
                    send(storage_server_list[index]->storage_server_socket, backup_r2, sizeof(st_request), 0);

                    response b1 = (response)malloc(sizeof(st_response));
                    int r1 = recv(storage_server_list[storage_server_id]->storage_server_socket, b1, sizeof(st_response), 0);
                    if(b1->response_type == ACK)
                    {
                        printf("File %s copied successfully in backup storage server 1\n", req->src_file_dir_name);
                    }
                    else
                    {
                        printf("Error in copying file in backup storage server 1\n");
                    }
                }
                // copy the file in the backup storage server 2
                if(storage_server_list[storage_server_count]->backup_storage_server2 != -1)
                {
                    int index = storage_server_list[storage_server_id]->backup_storage_server2;
                    if(storage_server_list[index]->backupfoldernumber2 == 1)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b1/%s",req->dest_path);
                    }
                    else if(storage_server_list[index]->backup_storage_server2 == 2)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b2/%s",req->dest_path);
                    }
                    else
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b3/%s",req->dest_path);
                    }
                    
                    printf("Copying file in backup storage server 2 %s\n", backup_full_path);
                    request backup_r1 = (request)malloc(sizeof(st_request));
                    backup_r1->request_type = COPY_FILE;
                    char copy_path[100];
                    char src_full_path[100];
                    strcpy(copy_path, req->src_path);
                    snprintf(src_full_path, sizeof(src_full_path), "main/%s", req->src_path);

                    strcpy(backup_r1->src_file_dir_name, req->src_file_dir_name);
                    strcpy(backup_r1->src_path, src_full_path);
                    strcpy(backup_r1->dest_path, backup_full_path);
                    strcpy(backup_r1->data, req->data);
                    strcpy(backup_r1->ip_for_copy, storage_server_list[index]->IP_Addr);
                    backup_r1->port_for_copy = storage_server_list[index]->Port_No;
                    backup_r1->socket = storage_server_list[index]->ss_socket;
                    send(storage_server_list[storage_server_id]->storage_server_socket, backup_r1, sizeof(st_request), 0);

                    request backup_r2 = (request)malloc(sizeof(st_request));
                    backup_r2->request_type = RECIEVE_FILE;
                    char dest_full_path[100];
                    snprintf(dest_full_path, sizeof(dest_full_path), "main/%s", req->src_path);
                    
                    char *token = strtok(copy_path, "/");
                    while(token != NULL)
                    {
                        strcpy(backup_r2->src_file_dir_name, token);
                        token = strtok(NULL, "/");
                    }

                    strcpy(backup_r2->src_path, full_path);
                    strcpy(backup_r2->dest_path, dest_full_path);
                    strcpy(backup_r2->data, req->data);
                    strcpy(backup_r2->ip_for_copy, storage_server_list[storage_server_id]->IP_Addr);
                    backup_r2->port_for_copy = storage_server_list[storage_server_id]->Port_No;
                    backup_r2->socket = storage_server_list[storage_server_id]->ss_socket;
                    send(storage_server_list[index]->storage_server_socket, backup_r2, sizeof(st_request), 0);
                    
                    response b2 = (response)malloc(sizeof(st_response));
                    int r1 = recv(storage_server_list[storage_server_id]->storage_server_socket, b2, sizeof(st_response), 0);
                    if(b2->response_type == ACK)
                    {
                        printf("File %s copied successfully in backup storage server 2\n", req->src_file_dir_name);
                    }
                    else
                    {
                        printf("Error in copying file in backup storage server 2\n");
                    }
                }
                
            }
        }
        else
        {
            response res = (response)malloc(sizeof(st_response));
            res->response_type = FILE_COPY_ERROR;
            strcpy(res->message, "Copy failed");
            strcpy(res->IP_Addr, "");
            res->Port_No = -1;
            send(client_id, res, sizeof(st_response), 0);
            logMessage(CLIENT_FLAG, client_id, *req, FILE_COPY_ERROR,0);
        }
        logMessage(CLIENT_FLAG, client_id, *req, PATH_FOUND,0);
    }
    else if(req->request_type == COPY_DIR)
    {
        if(storage_server_list[storage_server_id]->is_active == false)
        {
            response res = (response)malloc(sizeof(st_response));
            res->response_type = PATH_NOT_FOUND;
            strcpy(res->message, "Storage server is down, cannot copy directory");
            strcpy(res->IP_Addr, "");
            res->Port_No = -1;
            send(client_id, res, sizeof(st_response), 0);
            logMessage(CLIENT_FLAG, client_id, *req, PATH_NOT_FOUND,0);
            return;
        }
        int res5 = Get(req->dest_path);
        request des_r = (request)malloc(sizeof(st_request));
        printf("Copying directory\n");
        printf("res5: %d\n", res5);
        printf("storage_server_id: %d\n", storage_server_id);
        if(res5 == -1)
        {
            response res = (response)malloc(sizeof(st_response));
            res->response_type = COPY_TO_PATH_INVALID;
            strcpy(res->message, "Path not found");
            strcpy(res->IP_Addr, "");
            res->Port_No = -1;
            send(client_id, res, sizeof(st_response), 0);
            logMessage(CLIENT_FLAG, client_id, *req, COPY_TO_PATH_INVALID,0);
            return;
        } else if(res5 == storage_server_id){
            printf("Copying directory in the same storage server\n");

            r = (request)malloc(sizeof(st_request));
            r->request_type = COPY_TO_SAME_DIR;
            char copy_path[100];
            char full_path[100];
            strcpy(copy_path, req->src_path);
            snprintf(full_path, sizeof(full_path), "main/%s", req->src_path);
            char full_dest_path[100];
            snprintf(full_dest_path, sizeof(full_dest_path), "main/%s", req->dest_path);
            
            char* token = strtok(req->src_path, "/");
            while(token != NULL)
            {
                strcpy(r->src_file_dir_name, token);
                token = strtok(NULL, "/");
            }

            strcpy(r->src_file_dir_name, req->src_file_dir_name);
            strcpy(r->src_path, full_path);
            strcpy(r->dest_path, full_dest_path);  
            strcpy(r->ip_for_copy, storage_server_list[res5]->IP_Addr);
            r->port_for_copy = storage_server_list[res5]->Port_No;
            strcpy(r->data, req->data);    
            send(storage_server_list[storage_server_id]->storage_server_socket, r, sizeof(st_request), 0);

            response same_res = (response)malloc(sizeof(st_response));
            int r1 = recv(storage_server_list[storage_server_id]->storage_server_socket, same_res, sizeof(st_response), 0);
            if(same_res->response_type == ACK)
            {
                // insert the path into the hash table
                send(client_id, same_res, sizeof(st_response), 0);
            }
            else
            {
                response res = (response)malloc(sizeof(st_response));
                res->response_type = COPY_DIR_ERROR;
                strcpy(res->message, "Copy failed");
                strcpy(res->IP_Addr, "");
                res->Port_No = -1;
                send(client_id, res, sizeof(st_response), 0);
                logMessage(CLIENT_FLAG, client_id, *req, COPY_DIR_ERROR,0);
            }
            free(r);
            return;
        }
        
        r->request_type = COPY_DIR;
        // strcpy(r->src_path, req->src_path);
        // strcpy(r->dest_path, req->dest_path);
        char full_path[100];
        char full_dest_path[100];

        char copy_path[100];
        // tokenise the copy to get the directory name
        strcpy(copy_path, req->src_path);
        char *token = strtok(copy_path, "/");
        while(token != NULL)
        {
            strcpy(des_r->src_file_dir_name, token);
            token = strtok(NULL, "/");
        }

        snprintf(full_path, sizeof(full_path), "main/%s", req->src_path);
        snprintf(full_dest_path, sizeof(full_dest_path), "main/%s", req->dest_path);

        strcpy(r->src_file_dir_name, req->src_file_dir_name);
        strcpy(r->data, req->data);
        strcpy(r->src_path, full_path);
        strcpy(r->dest_path, full_dest_path);
        strcpy(r->ip_for_copy, storage_server_list[res5]->IP_Addr);
        r->port_for_copy = storage_server_list[res5]->Port_No;
        r->socket = storage_server_list[res5]->ss_socket;

        printf("Sending request to source storage directory\n");
        printf("%d\n", storage_server_id);
        send(storage_server_list[storage_server_id]->storage_server_socket, r, sizeof(st_request), 0);

        // sleep(1);
        // request for the destination storage server
        des_r->request_type = RECIEVE_DIR;
        full_path[100];
        full_dest_path[100];
        snprintf(full_path, sizeof(full_path), "main/%s", req->src_path);
        snprintf(full_dest_path, sizeof(full_dest_path), "main/%s", req->dest_path);

        strcpy(des_r->data, req->data);
        strcpy(des_r->src_path, full_path);
        strcpy(des_r->dest_path, full_dest_path);
        // strcpy(des_r->ip_for_copy, storage_server_list[storage_server_id]->IP_Addr);
        // des_r->port_for_copy = storage_server_list[storage_server_id]->Port_No;
        strcpy(des_r->ip_for_copy, storage_server_list[res5]->IP_Addr);
        des_r->port_for_copy = storage_server_list[res5]->Port_No;

        // maybe error check it
        des_r->socket = storage_server_list[res5]->ss_socket;

        printf("Sending request to reciever storage directory\n");
        printf("%d\n", res5);
        send(storage_server_list[res5]->storage_server_socket, des_r, sizeof(st_request), 0);
        
        response resp1 = (response)malloc(sizeof(st_response));
        int r1 = recv(storage_server_list[storage_server_id]->storage_server_socket, resp1, sizeof(st_response), 0);
        
        if(resp1->response_type == ACK)
        {
            // insert the path into the hash table
            char s[100];
            snprintf(s, sizeof(s), "%s/%s", req->dest_path, des_r->src_file_dir_name);
            printf("Inserted path: %s in the hash table\n", s);
            if(Get(s) == -1)
                Insert(s, res5);
            send(client_id, resp1, sizeof(st_response), 0);

            // now as the directory has got copied so also copy it in both the backups of that storage server
            if(storage_server_list[storage_server_id]->is_backed_up == true)
            {
                char backup_full_path[MAX_DATA_LENGTH];
                if(storage_server_list[storage_server_id]->backup_storage_server1 != -1)
                {
                    int index = storage_server_list[storage_server_id]->backup_storage_server1;
                    if(storage_server_list[index]->backupfoldernumber1 == 1)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b1/%s",req->dest_path);
                    }
                    else if(storage_server_list[index]->backup_storage_server1 == 2)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b2/%s",req->dest_path);
                    }
                    else
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b3/%s",req->dest_path);
                    }
                    
                    printf("Copying directory in backup storage server 1 %s\n", backup_full_path);
                    request backup_r1 = (request)malloc(sizeof(st_request));
                    backup_r1->request_type = COPY_DIR;
                    char copy_path[100];
                    char src_full_path[100];
                    strcpy(copy_path, req->src_path);
                    snprintf(src_full_path, sizeof(src_full_path), "main/%s", req->src_path);

                    strcpy(backup_r1->src_file_dir_name, req->src_file_dir_name);
                    strcpy(backup_r1->src_path, src_full_path);
                    strcpy(backup_r1->dest_path, backup_full_path);
                    strcpy(backup_r1->data, req->data);
                    strcpy(backup_r1->ip_for_copy, storage_server_list[index]->IP_Addr);
                    backup_r1->port_for_copy = storage_server_list[index]->Port_No;
                    backup_r1->socket = storage_server_list[index]->ss_socket;
                    send(storage_server_list[storage_server_id]->storage_server_socket, backup_r1, sizeof(st_request), 0);

                    request backup_r2 = (request)malloc(sizeof(st_request));
                    backup_r2->request_type = RECIEVE_DIR;
                    char dest_full_path[100];
                    snprintf(dest_full_path, sizeof(dest_full_path), "main/%s", req->src_path);
                    
                    char *token = strtok(copy_path, "/");
                    while(token != NULL)
                    {
                        strcpy(backup_r2->src_file_dir_name, token);
                        token = strtok(NULL, "/");
                    }

                    strcpy(backup_r2->src_path, full_path);
                    strcpy(backup_r2->dest_path, dest_full_path);
                    strcpy(backup_r2->data, req->data);
                    strcpy(backup_r2->ip_for_copy, storage_server_list[storage_server_id]->IP_Addr);
                    backup_r2->port_for_copy = storage_server_list[storage_server_id]->Port_No;
                    backup_r2->socket = storage_server_list[storage_server_id]->ss_socket;
                    send(storage_server_list[storage_server_id]->storage_server_socket, backup_r2, sizeof(st_request), 0);

                    response b1 = (response)malloc(sizeof(st_response));
                    int r1 = recv(storage_server_list[storage_server_id]->storage_server_socket, b1, sizeof(st_response), 0);
                    if(b1->response_type == ACK)
                    {
                        printf("Directory %s copied successfully in backup storage server 1\n", req->src_file_dir_name);
                    }
                    else
                    {
                        printf("Error in copying directory in backup storage server 1\n");
                    }
                }
                if(storage_server_list[storage_server_id]->backup_storage_server2 != -1)
                {
                    int index = storage_server_list[storage_server_id]->backup_storage_server2;
                    if(storage_server_list[index]->backupfoldernumber2 == 1)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b1/%s",req->dest_path);
                    }
                    else if(storage_server_list[index]->backup_storage_server2 == 2)
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b2/%s",req->dest_path);
                    }
                    else
                    {
                        snprintf(backup_full_path, sizeof(backup_full_path), "b3/%s",req->dest_path);
                    }
                    
                    printf("Copying directory in backup storage server 2 %s\n", backup_full_path);
                    request backup_r1 = (request)malloc(sizeof(st_request));
                    backup_r1->request_type = COPY_DIR;
                    char copy_path[100];
                    char src_full_path[100];
                    strcpy(copy_path, req->src_path);
                    snprintf(src_full_path, sizeof(src_full_path), "main/%s", req->src_path);

                    strcpy(backup_r1->src_file_dir_name, req->src_file_dir_name);
                    strcpy(backup_r1->src_path, src_full_path);
                    strcpy(backup_r1->dest_path, backup_full_path);
                    strcpy(backup_r1->data, req->data);
                    strcpy(backup_r1->ip_for_copy, storage_server_list[index]->IP_Addr);
                    backup_r1->port_for_copy = storage_server_list[index]->Port_No;
                    backup_r1->socket = storage_server_list[index]->ss_socket;
                    send(storage_server_list[storage_server_id]->storage_server_socket, backup_r1, sizeof(st_request), 0);

                    request backup_r2 = (request)malloc(sizeof(st_request));
                    backup_r2->request_type = RECIEVE_DIR;
                    char dest_full_path[100];
                    snprintf(dest_full_path, sizeof(dest_full_path), "main/%s", req->src_path);
                    
                    char *token = strtok(copy_path, "/");
                    while(token != NULL)
                    {
                        strcpy(backup_r2->src_file_dir_name, token);
                        token = strtok(NULL, "/");
                    }

                    strcpy(backup_r2->src_path, full_path);
                    strcpy(backup_r2->dest_path, dest_full_path);
                    strcpy(backup_r2->data, req->data);
                    strcpy(backup_r2->ip_for_copy, storage_server_list[storage_server_id]->IP_Addr);
                    backup_r2->port_for_copy = storage_server_list[storage_server_id]->Port_No;
                    backup_r2->socket = storage_server_list[storage_server_id]->ss_socket;
                    send(storage_server_list[storage_server_id]->storage_server_socket, backup_r2, sizeof(st_request), 0);

                    response b2 = (response)malloc(sizeof(st_response));
                    int r1 = recv(storage_server_list[storage_server_id]->storage_server_socket, b2, sizeof(st_response), 0);
                    if(b2->response_type == ACK)
                    {
                        printf("Directory %s copied successfully in backup storage server 2\n", req->src_file_dir_name);
                    }
                    else
                    {
                        printf("Error in copying directory in backup storage server 2\n");
                    }
                }
            }
        }
        else
        {
            response res1 = (response)malloc(sizeof(st_response));
            res1->response_type = COPY_DIR_ERROR;
            strcpy(res1->message, "Copy failed");
            strcpy(res1->IP_Addr, "");
            res1->Port_No = -1;
            send(client_id, res1, sizeof(st_response), 0);
            logMessage(CLIENT_FLAG, client_id, *req, COPY_DIR_ERROR,0);
        }
        logMessage(CLIENT_FLAG, client_id, *req, PATH_FOUND,0);
    }
    else
    {
        response res = (response)malloc(sizeof(st_response));
        res->response_type = INVALID_REQUEST;
        strcpy(res->message, "Invalid request");
        strcpy(res->IP_Addr, "");
        res->Port_No = -1;
        send(client_id, res, sizeof(st_response), 0);
        add_to_cache(req, res, -1);
        logMessage(CLIENT_FLAG, client_id, *req, INVALID_REQUEST,0);
    }
    free(r);
}