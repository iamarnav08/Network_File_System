#include "headers.h" // Replace with your actual header file

#define BUFFER_SIZE 1024
#define MAX_TOKENS 10
#define MAX_CLIENTS 100

// Main function
int main()
{
    printf(BLUE_COLOR "For input-related doubts, type MAN\n" RESET_COLOR);

    char input[3000];
    char *tokens[MAX_TOKENS];

    while (1)
    {
        printf(YELLOW_COLOR "\n------- Enter operation you want to do -------\n" RESET_COLOR);
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        // Tokenize and store in the tokens array
        int token_count = 0;
        char *token = strtok(input, " ");
        while (token != NULL && token_count < MAX_TOKENS)
        {
            tokens[token_count++] = token;
            token = strtok(NULL, " ");
        }

        if (token_count == 0)
        {
            printf(RED_COLOR "No operation entered. Try again.\n" RESET_COLOR);
            continue;
        }

        // Process input commands
        if (strcmp("READ", tokens[0]) == 0 && token_count == 2)
        {
            reading_operation(tokens[1]); // Implement this function
        }
        else if (strcmp("WRITE", tokens[0]) == 0 && token_count == 2)
        {
            printf(BLUE_COLOR "Do you want this write to be done on a priority basis (yes\\no)? " RESET_COLOR);

            char ans[10]; // Buffer to store the user's response

            // Use fgets to read the input, ensuring the newline is handled
            if (fgets(ans, sizeof(ans), stdin) != NULL)
            {
                // Remove the newline character if present
                size_t len = strlen(ans);
                if (len > 0 && ans[len - 1] == '\n')
                {
                    ans[len - 1] = '\0';
                }

                // Convert the input to lowercase for easier comparison
                for (int i = 0; ans[i]; i++)
                {
                    ans[i] = tolower(ans[i]);
                }

                if (strcmp(ans, "yes") == 0)
                {
                    // User wants priority, pass 1 for priority
                    writing_append_operation(tokens[1], 1);
                }
                else if (strcmp(ans, "no") == 0)
                {
                    // User doesn't want priority, pass 0 for normal
                    writing_append_operation(tokens[1], 0);
                }
                else
                {
                    printf("Invalid input. Please enter 'yes' or 'no'.\n");
                }
            }
            else
            {
                printf("Error reading input.\n");
            }
        }

        else if (strcmp("CREATE", tokens[0]) == 0 && token_count == 4)
        {
            if (strcmp("FOLDER", tokens[1]) == 0)
            {
                create_operation(tokens[2], tokens[3], CREATE_FOLDER); // Implement this function
            }
            else if (strcmp("FILE", tokens[1]) == 0)
            {
                create_operation(tokens[2], tokens[3], CREATE_FILE); // Implement this function
            }
        }
        else if (strcmp("DELETE", tokens[0]) == 0 && token_count == 4)
        {
            if (strcmp("FOLDER", tokens[1]) == 0)
            {
                delete_operation(tokens[2], tokens[3], DELETE_FOLDER); // Implement this function
            }
            else if (strcmp("FILE", tokens[1]) == 0)
            {
                delete_operation(tokens[2], tokens[3], DELETE_FILE); // Implement this function
            }
        }
        else if (strcmp("COPY", tokens[0]) == 0 && token_count == 4)
        {
            if (strcmp("FOLDER", tokens[1]) == 0)
            {
                copy_operation(COPY_FOLDER, tokens[2], tokens[3]); // Implement this function
            }
            else
            {
                copy_operation(COPY_FILE, tokens[2], tokens[3]); // Implement this function
            }
        }
        else if (strcmp("STREAM", tokens[0]) == 0 && token_count == 2)
        {
            stream(tokens[1]); // Implement this function
        }
        else if (strcmp("INFO", tokens[0]) == 0 && token_count == 2)
        {
            info(tokens[1]); // Implement this function
        }
        else if (strcmp("LIST", tokens[0]) == 0)
        {
            list(); // Implement this function
        }
        else if (strcmp("EXIT", tokens[0]) == 0)
        {
            break;
        }
        else
        {
            printf(RED_COLOR "Invalid operation. Try again.\n" RESET_COLOR);
        }
    }

    return 0;
}
