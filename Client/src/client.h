#ifndef CLIENT_H
#define CLIENT_H

/* INCLUDE */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

/* CONSTANT DECLARATION*/
#define BUFFER_SIZE 1024

/* FUNCTIONS DECLARATION */
void check_dir(char *path); // check if a directory exists or else create it 
char* concatenate_strings(char* str1, char* str2, int flag); // take two string pointer and return a final one
int not_enough_space(char *path, size_t file_size); // return 1 if there isn't enough space or if an error occurred else 0
void client_w(int client_socket, char *remote_file_path, char *file_path); // this function manage the -w command client-side
void client_r(int client_socket, char *remote_file_path, char *file_path); // this function manage the -r command client-side
void client_l(int client_socket, char *remote_file_path); // this function manage the -l command client-side

#endif /* CLIENT_H */