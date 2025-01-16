#include "headers.h"

#define MAX_TOKENS 100 // Define maximum number of tokens

void list()
{
    int client_socket = connect_with_ns(); // Establish connection to the namespace server
    if (client_socket == -1) {
        perror("Failed to connect to namespace server");
        return;
    }

    st_request packet;
    packet.request_type = 12; // Set the request type to LIST

    // Send the LIST request to the server
    ssize_t bytes_sent = send(client_socket, &packet, sizeof(st_request), 0);
    // printf("%d\n",sizeof(st_request));
    // packet.
    printf("%ld\n",bytes_sent);
    if (bytes_sent == -1)
    {
        perror("Send failed");
        close(client_socket); // Close the socket if send fails
        return;
    }

    st_response response;

    // Receive the server's response
    memset(&response,0,sizeof(st_response));
    int bytes_received = recv(client_socket, &response, sizeof(st_response), 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            printf("Server closed the connection.\n");
        } else {
            perror("Receive failed");
        }
        close(client_socket); // Close the socket on error or disconnection
        return;
    }

    // Check the response type and process the message
    if (response.response_type == 15)
    {
        char *tokens[MAX_TOKENS]; // Array to hold tokens
        int token_count = 0;

        char *token = strtok(response.message, ";"); // Tokenize the message using ";" as the delimiter
        while (token != NULL && token_count < MAX_TOKENS)
        {
            tokens[token_count++] = token; // Store the token
            token = strtok(NULL, ";");    // Get the next token
        }

        if (token_count == 0)
        {
            printf(RED_COLOR"No accessible paths to print\n"RESET_COLOR);
        }
        else
        {
            printf(GREEN_COLOR"Accessible paths:\n"RESET_COLOR);

            for (int i = 0; i < token_count; i++) {
                printf("%s\n", tokens[i]); // Print each stored token
            }
        }
    }
    else
    {
        printf(RED_COLOR"List Request Not successful: %s\n"RESET_COLOR, response.message);
    }

    close(client_socket); // Close the socket after completing the operation
}
