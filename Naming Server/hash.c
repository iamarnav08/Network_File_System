#include "headers.h"

// Create the hash table
node* Create_hashtable() {
    node* temp = (node*)calloc(itablesize, sizeof(node));
    return temp;
}

// Free the hash table and its nodes
void Free_hashtable() {
    for (int i = 0; i < itablesize; ++i) {
        node cur = hashtable[i];
        while (cur) {
            node to_free = cur;
            cur = cur->next;
            free(to_free->path);
            free(to_free);
        }
    }
    free(hashtable);
}

// Create a hash value for the given path
unsigned long create_hash(const char* x) {
    unsigned long hash = 5381; // A common hashing seed
    int c;
    while ((c = *x++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % itablesize;
}

// Insert a path into the hash table
void Insert(const char* path, int s_i) {
    int pos = create_hash(path);

    // Create a new node
    node new_node = (node)malloc(sizeof(st_node));
    new_node->path = strdup(path);
    new_node->len = strlen(path);
    new_node->s_index = s_i;
    new_node->next = hashtable[pos];

    // Insert at the beginning of the chain
    hashtable[pos] = new_node;
}

// Get the storage index of a path
int Get(const char* path) {
    int pos = create_hash(path);
    node cur = hashtable[pos];
    while (cur) {
        if (strcmp(cur->path, path) == 0)
            return cur->s_index;
        cur = cur->next;
    }
    return -1; // Path not found
}

// Delete a path from the hash table
void Delete(const char* path) {
    int pos = create_hash(path);
    node cur = hashtable[pos];
    node prev = NULL;

    while (cur) {
        if (strcmp(cur->path, path) == 0) {
            if (prev) {
                prev->next = cur->next;
            } else {
                hashtable[pos] = cur->next;
            }

            free(cur->path);
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

// response Print_all_paths() {
//     response req = (response)malloc(sizeof(st_response));
//     req->response_type = 3;
//     for (int i = 0; i < itablesize; ++i) {
//         node cur = hashtable[i];
//         while (cur) {
//              if (strlen(req->message) + strlen(cur->path) + 2 > sizeof(req->message)) {
//                 // Handle the case where the message buffer is too small
//                 // You might want to reallocate or handle the error
//                 fprintf(stderr, "Error: req->message buffer is too small\n");
//                 return req;
//             }
//             strcat(req->message, cur->path);
//             strcat(req->message, ";");
//             cur = cur->next;
//         }
//     }
//     size_t len = strlen(req->message);
//     if (len > 0 && req->message[len - 1] == ';') {
//         req->message[len - 1] = '\0';
//     }
//     return req;
// }

response Print_all_paths() {
    response req = (response)malloc(sizeof(st_response));
    req->response_type = 3;
    for (int i = 0; i < itablesize; ++i) {
        node cur = hashtable[i];
        while (cur) {
             if (strlen(req->message) + strlen(cur->path) + 2 > sizeof(req->message)) {
                // Handle the case where the message buffer is too small
                // You might want to reallocate or handle the error
                fprintf(stderr, "Error: req->message buffer is too small\n");
                return req;
            }
            strcat(req->message, cur->path);
            strcat(req->message, ";");
            cur = cur->next;
        }
    }
    size_t len = strlen(req->message);
    if (len > 0 && req->message[len - 1] == ';') {
        req->message[len - 1] = '\0';
    }
    return req;
}

void remove_paths_from_hash(int index)
{
    // delete all the nodes that have the s_index = index
    for(int i=0;i<itablesize;i++)
    {
        node cur = hashtable[i];
        node prev = NULL;

        while (cur != NULL) {
            if (cur->s_index == index) {
                if (prev == NULL) {
                    // Node to be deleted is the first node in the list
                    hashtable[i] = cur->next;
                } else {
                    // Node to be deleted is not the first node
                    prev->next = cur->next;
                }
                node temp = cur;
                cur = cur->next;
                free(temp); // Free the node
            } else {
                prev = cur;
                cur = cur->next;
            }
        }
    }
}