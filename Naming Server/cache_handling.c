#include "headers.h"

void clear_cache(){
    for(int i = 0; i < cache->curr_size; i++){
        cache->cache_arr[i].ss_ind = -1;
        bzero(cache->cache_arr[i].key, sizeof(st_request));
        bzero(cache->cache_arr[i].value, sizeof(st_response));
    }
    cache->curr_size = 0;
}

// helper finctions to copy data from one request/response to another to cache it
void copy_request(request dest, request src){
    // add all the attributes of the request structure here
    dest->request_type = src->request_type;
    strcpy(dest->data, src->data);
    strcpy(dest->src_path, src->src_path);
    strcpy(dest->src_file_dir_name, src->src_file_dir_name);
    strcpy(dest->dest_path, src->dest_path);
    strcpy(dest->ip_for_copy, src->ip_for_copy);
    dest->port_for_copy = src->port_for_copy;
    dest->socket = src->socket;
    dest->sync = src->sync;
}

void copy_response(response dest, response src){
    dest->response_type = src->response_type;
    strcpy(dest->message, src->message);
    strcpy(dest->IP_Addr, src->IP_Addr);
    dest->Port_No = src->Port_No;
}

my_cache* initialize_cache(){
    my_cache* cache = (my_cache*)malloc(sizeof(my_cache));
    for(int i=0;i<15;i++){
        cache->cache_arr[i].key = (request)malloc(sizeof(st_request));
        cache->cache_arr[i].value = (response)malloc(sizeof(st_response));
    }
    cache->curr_size = 0;
    return cache;
}   

bool is_cache_full(){
    return cache->curr_size == CACHE_SIZE;
}

bool is_cache_empty(){
    return cache->curr_size == 0;
}

void move_to_front(int index){
    cache_entry temp = cache->cache_arr[index];
    for(int i = index; i > 0; i--){
        cache->cache_arr[i] = cache->cache_arr[i-1];
    }
    cache->cache_arr[0] = temp;
}

void add_to_cache(request key, response value,int ss_ind){
    if(is_cache_full()){
        remove_from_cache(cache->cache_arr[cache->curr_size-1].key);
    }
    move_to_front(cache->curr_size);
    cache->cache_arr[0].ss_ind = ss_ind;
    copy_request(cache->cache_arr[0].key, key);
    copy_response(cache->cache_arr[0].value, value);
    cache->curr_size++;
}

response get_from_cache(request key){
    for(int i = 0; i < cache->curr_size; i++){
        if(strcmp(cache->cache_arr[i].key->src_path, key->src_path) == 0 && strcmp(cache->cache_arr[i].key->src_file_dir_name, key->src_file_dir_name) == 0 && cache->cache_arr[i].key->request_type == key->request_type){
            move_to_front(i);
            return cache->cache_arr[0].value;
        }
    }
    return NULL;
}

void remove_from_cache(request key){
    // check if present and delete with making the ss_ind -1 not freeing the memory
    for(int i = 0; i < cache->curr_size; i++){
        if(strcmp(cache->cache_arr[i].key->src_path, key->src_path) == 0 && strcmp(cache->cache_arr[i].key->src_file_dir_name, key->src_file_dir_name) == 0 && cache->cache_arr[i].key->request_type == key->request_type){
            cache->cache_arr[i].ss_ind = -1;
            bzero(cache->cache_arr[i].key, sizeof(st_request));
            bzero(cache->cache_arr[i].value, sizeof(st_response));
            return;
        }
    }
}

void delete_from_cache_ssid(int ss_id){
    int j = 0; // Index to track the position of the next valid element

    // First pass: Mark the elements to be deleted
    for(int i = 0; i < cache->curr_size; i++){
        if(cache->cache_arr[i].ss_ind == ss_id){
            cache->cache_arr[i].ss_ind = -1;
            bzero(cache->cache_arr[i].key, sizeof(st_request));
            bzero(cache->cache_arr[i].value, sizeof(st_response));
        }
    }

    // Second pass: Move valid elements to the front
    for(int i = 0; i < cache->curr_size; i++){
        if(cache->cache_arr[i].ss_ind != -1){
            if(i != j){
                cache->cache_arr[j] = cache->cache_arr[i];
            }
            j++;
        }
    }

    // Update the current size of the cache
    cache->curr_size = j;
}

void Free_cache(){
    for(int i=0;i<15;i++){
        free(cache->cache_arr[i].key);
        free(cache->cache_arr[i].value);
    }
    free(cache);
}
