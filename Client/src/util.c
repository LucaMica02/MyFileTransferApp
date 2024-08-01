#include "client.h"
#include <sys/statvfs.h>

// take two string pointer and return a final one
char* concatenate_strings(char* str1, char* str2, int flag) {
    if (str1 == NULL || str2 == NULL) {
        return NULL;
    }

    // determine the lengths 
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    size_t result_size = len1 + len2 + 2; // +2 for '/' and the null terminator

    // allocate memory for the concatenated string
    char *result = malloc(result_size);
    if (result == NULL) {
        perror("Error allocating memory");
        exit(EXIT_FAILURE);
    }

    // copy the first string into the result
    strncpy(result, str1, len1);
    result[len1] = '\0'; // terminate the string

    if (flag != 0) { // add '/' 
        strcat(result, "/");
    }

    // add the second string to the result
    strncat(result, str2, len2);

    return result;
}

// check if a directory exists or else create it 
void check_dir(char *path) {
    struct stat st; // to get information about the path
    if (stat(path, &st) == 0) { 
        if (!(S_ISDIR(st.st_mode))) { // exists but isn't a directory
            printf("Error: unable to create directory, file already exists");
            exit(EXIT_FAILURE);
        }
    } else {
        int status = mkdir(path, 0700); // 0700 gives read, write, execute permissions to owner
        if (status != 0) {
            perror("Error creating directory");
            exit(EXIT_FAILURE);
        }
    }
}

// return 1 if there isn't enough space or if an error occurred else 0
int not_enough_space(char *path, size_t file_size) {
    struct statvfs stat; // to get information about the file system

    if (strcmp(path, "") == 0) { // to avoid error
      path = "/";
    }

    if (statvfs(path, &stat) != 0) {
        perror("statvfs error");
        return 1; // space cannot be determined
    }

    // number of block available for block size
    unsigned long long available_space = stat.f_bsize * stat.f_bavail;

    if (available_space >= file_size) { // there is enough space
        return 0;
    }

    return 1; // there isn't enough space
}