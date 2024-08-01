#ifndef SERVER_H
#define SERVER_H

/* INCLUDE */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

/* CONSTANTS DECLARATION*/
#define BUFFER_SIZE 1024
#define MAX_OPEN_FILES 1000
#define MAX_FILE_SIZE 1073741824 // 1 Gb in byte

/* STRUCT DECLARATION*/
typedef struct {
    int client_socket;
    char *curr_path;
} ThreadArgs;

/* FUNCTIONS DECLARATION */
void* handle_client(void *arg); // this function manage the comunication with a client
void server_l(int client_socket, char *curr_path); // this function manage the -l command server-side
void server_w(int client_socket, char *curr_path); // this function manage the -w command server-side
void server_r(int client_socket, char *curr_path); // this function manage the -r command server-side
void wait_until_file_available(); // remain in sleep status until the open file are lesser then the max open files
int count_open_files(); // count the number of open files in the current process
char* concatenate_strings(char* str1, char* str2, int flag); // take two string pointers and return a final one
void check_dir(char *path); // check if a directory exists or else create it 
int detect_path_traversal(char *path); // take a path and return 1 if detect path traversal else 0
int not_enough_space(char *path, size_t file_size); // return 1 if there isn't enough space or if an error occurred else 0

#endif /* SERVER_H */