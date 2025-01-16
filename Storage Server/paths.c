#include "functions.h"

void traverse_directory(const char *base_dir, const char *relative_dir, char **result, size_t *result_size, size_t *len) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char full_path[MAX_COPY_LEN];
    char relative_path[MAX_COPY_LEN];

    // Construct the full path to the current directory
    snprintf(full_path, sizeof(full_path), "%s/%s", base_dir, relative_dir);

    dir = opendir(full_path);
    if (!dir) {
        // perror("Unable to open directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Ignore "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Construct full path and relative path
        snprintf(full_path, sizeof(full_path), "%s/%s", base_dir, relative_dir);
        snprintf(relative_path, sizeof(relative_path), "%s/%s", relative_dir, entry->d_name);

        // Remove the leading '/' in relative_path if present
        char *trimmed_path = relative_path;
        if (trimmed_path[0] == '/') {
            trimmed_path++;
        }

        // Get file information
        if (stat(full_path, &statbuf) == 0) {
            // Append the trimmed relative path to the result string
            size_t name_len = strlen(trimmed_path);
            if (*len + name_len + 2 > *result_size) { // +2 for ';' and '\0'
                *result_size *= 2; // Double the buffer size
                char *new_result = realloc(*result, *result_size);
                if (!new_result) {
                    perror("Memory reallocation failed");
                    free(*result);
                    closedir(dir);
                    return;
                }
                *result = new_result;
            }

            strcat(*result, trimmed_path);
            *len += name_len;

            // Add ';' as a separator
            strcat(*result, ";");
            *len += 1;

            // If it's a directory, recurse
            if (S_ISDIR(statbuf.st_mode)) {
                traverse_directory(base_dir, trimmed_path, result, result_size, len);
            }
        }
    }

    closedir(dir);
}

char* get_all_files_and_dirs_recursive(const char *base_dir) {
    char *result = malloc(MAX_PATH_LEN * MAX_PATHS * sizeof(char)); // Initial allocation
    size_t result_size = MAX_PATH_LEN;
    size_t len = 0;

    if (!result) {
        perror("Memory allocation failed");
        return NULL;
    }

    result[0] = '\0'; // Initialize as empty string

    // Start traversing the directory with an empty relative path
    traverse_directory(base_dir, "", &result, &result_size, &len);

    // Remove trailing ';' if present
    if (len > 0 && result[len - 1] == ';') {
        result[len - 1] = '\0';
    }

    return result;
}
