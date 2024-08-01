#include "server.h"
#include <sys/statvfs.h> // for statvfs struct to get info about the file system
#include <dirent.h> // for dirent struct to manage directory entry

extern pthread_mutex_t mutex; // mutex for manage the access to read the free memory space

// remain in sleep status until the open file are lesser then the max open files
// to avoid error from too many open files
void wait_until_file_available() {
    while (count_open_files() > MAX_OPEN_FILES) {
        usleep(100000); // 100000 microsecondi = 0.1 secondi
    }
}

// count the number of open files in the current process
int count_open_files() {
    int count = 0;
    struct dirent *entry; // for manage the directory entry
    // open this directory that contain the symbolic link to the file 
    // descriptors opened by the current process
    DIR *dp = opendir("/proc/self/fd"); 

    if (dp == NULL) {
        perror("opendir");
        return -1;
    }

    // count the entry
    while ((entry = readdir(dp)) != NULL) {
        count++;
    }

    closedir(dp); // close the directory

    // subtract 2 to remove the count of the current directory and parent directory
    return count - 2;
}

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

// take a path and return 1 if detect path traversal else 0
int detect_path_traversal(char *path) {
    char curr[4] = "";

    // iterate throught the string searching for path traversal
    for (int i = 0; i < strlen(path)-2; i++) {
        curr[0] = path[i];
        curr[1] = path[i+1];
        curr[2] = path[i+2];
        curr[3] = '\0';
        if ((strcmp(curr, "../") == 0) || (strcmp(curr, "/..") == 0) || (strcmp(curr, "\\..") == 0) || (strcmp(curr, "..\\") == 0)) {
            return 1; // there is path traversal
        }
    }

    return 0; // there isn't path traversal
}

// return 1 if there isn't enough space or if an error occurred else 0
int not_enough_space(char *path, size_t file_size) {
    pthread_mutex_lock(&mutex); // lock the mutex
    struct statvfs stat; // to get information about the file system

    if (strcmp(path, "") == 0) { // to avoid error
      path = "/";
    }

    if (statvfs(path, &stat) != 0) {
        perror("statvfs error");
        pthread_mutex_unlock(&mutex); // unlock the mutex
        return 1; // space cannot be determined
    }

    // number of block available for block size
    unsigned long long available_space = stat.f_bsize * stat.f_bavail;

    if (available_space >= file_size) { // there is enough space
        pthread_mutex_unlock(&mutex); // unlock the mutex
        return 0;
    }

    pthread_mutex_unlock(&mutex); // unlock the mutex
    return 1; // there isn't enough space
}