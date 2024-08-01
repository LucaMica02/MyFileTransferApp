#include "client.h"
#include <fcntl.h> // for open flags

// this function manage the -r command client-side
void client_r (int client_socket, char *remote_file_path, char *file_path) {
  char *token; // used for the function strtok()
  char *last_symb; // used for find the last occurency of file path
  char *curr_path = ""; // used for check or create the path
  char *copy_path;
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;
  int flag = 0;
  struct stat st;

  // send the length of the file path to the server
  uint32_t path_length = htonl(strlen(remote_file_path)); // convert to network byte order
  if (send(client_socket, &path_length, sizeof(path_length), 0) == -1) {
    perror("Error in send()");
    close(client_socket);
    exit(EXIT_FAILURE);
  }

  // send the file path to the server
  if (send(client_socket, remote_file_path, strlen(remote_file_path), 0) == -1) {
    perror("Error in send()");
    close(client_socket);
    exit(EXIT_FAILURE);
  }

  // receive the length of response
  uint32_t response_length;
  bytes_read = recv(client_socket, &response_length, sizeof(response_length), 0);
  if (bytes_read <= 0) {
    perror("Error receiving file path length");
    close(client_socket);
    exit(EXIT_FAILURE);
  }
  response_length = ntohl(response_length); // convert from network byte order

  char *response = malloc(response_length + 1); // memory allocation for receive the file path
  if (response == NULL) {
    perror("Error allocating memory");
    exit(EXIT_FAILURE);
  }

  // receive the response from the server
  recv(client_socket, response, response_length, 0);
  if (response_length > 0) {
    if (strcmp(response, "OK") == 0) { // if response is OK

        // check if the file path isn't a directory
        if (stat(file_path, &st) == 0 && S_ISDIR(st.st_mode)) { 
            printf("Error: unable to write the file, it is a directory\n");
            close(client_socket);
            free(response);
            exit(EXIT_FAILURE);
        }

        // check if the path exists or else create it
        copy_path = strdup(file_path); // copy the file path so as not to corrupt the original string
        last_symb = strrchr(copy_path, '/'); // find the last occurrency of '/'
        if (last_symb != NULL) {
            *last_symb = '\0'; // if there is truncate the string 
            token = strtok(copy_path, "/");
            while (token != NULL) {
                curr_path = concatenate_strings(curr_path, token, flag); // build the path
                check_dir(curr_path); // check if the directory exists or create it
                token = strtok(NULL, "/");
                flag++;
            }
        }
        free(copy_path);

        // receive file length
        uint32_t netFileSize;
        if (recv(client_socket, &netFileSize, sizeof(netFileSize), 0) <= 0) {
            perror("Error receiving file size");
            close(client_socket);
            free(response);
            exit(EXIT_FAILURE);
        }
        uint32_t fileSize = ntohl(netFileSize); // convert from network byte order

        // check if there is enough space on the file system
        if (not_enough_space(curr_path, fileSize) == 1) {
          printf("Not enough memory space\n");
          close(client_socket);
          free(response);
          exit(EXIT_FAILURE);
        }

        // open the file in write mode or create it 
        int file_descriptor = open(file_path, O_WRONLY | O_CREAT, 0700); // read, write and execute permissions to the owner
        if (file_descriptor == -1) {
            perror("Error opening file for writing");
            close(client_socket);
            free(response);
            exit(EXIT_FAILURE);
        }

        // truncates the file to zero bytes to avoid corrupted files
        if (ftruncate(file_descriptor, 0) == -1) {
            perror("Error truncating file");
            close(file_descriptor);
            close(client_socket);
            free(response);
            exit(EXIT_FAILURE);
        }

        // receive the bytes stream and write it on the file
        uint32_t totalBytesReceived = 0;
        while (totalBytesReceived < fileSize && (bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
            write(file_descriptor, buffer, bytes_read);
            totalBytesReceived += bytes_read;
        }

        // if an error occurs in receive truncates the file to zero bytes to avoid corrupted files
        if (bytes_read == -1) {
            perror("Error in recv()");
            ftruncate(file_descriptor, 0);
            close(file_descriptor);
            close(client_socket);
            free(response);
            exit(EXIT_FAILURE);
        }

        close(file_descriptor);
        printf("File received successfully.\n");
    } else { // if error in response print it
        printf("%s\n", response);
    } 
  } else { // if no response from the server
    printf("No response from the server.\n");
  }

  free(response);
}