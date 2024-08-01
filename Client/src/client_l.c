#include "client.h"

// this function manage the -l command client-side
void client_l(int client_socket, char *remote_file_path) {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;

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
        while (bytes_read > 0) {

            // receive the line
            bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_read == -1) {
                perror("Error in recv()");
                close(client_socket);
                free(response);
                exit(EXIT_FAILURE);
            }

            // print the output
            if (bytes_read > 0) {
                printf("%s", buffer);
            }
        }
        printf("Command execute successfully.\n");
    } else { // if error in response print it
        printf("%s\n", response);
    } 
  } else { // if no response from the server
    printf("No response from the server.\n");
  }

  free(response);
}