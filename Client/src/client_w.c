#include "client.h"

// this function manage the -w command client-side
void client_w(int client_socket, char *remote_file_path, char *file_path) {
  FILE *file_to_send;
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;

  // send the length of the file path to the server
  uint32_t path_length = htonl(strlen(remote_file_path)); // convert to network byte order
  if (send(client_socket, &path_length, sizeof(path_length), 0) == -1) {
    perror("Error in send()");
    exit(EXIT_FAILURE);
  }

  // send the file path to the server
  if (send(client_socket, remote_file_path, strlen(remote_file_path), 0) == -1) {
    perror("Error in send()");
    exit(EXIT_FAILURE);
  }

  // open the file to send
  file_to_send = fopen(file_path, "rb");
  if (file_to_send == NULL) {
    perror("Error opening file for reading");
    exit(EXIT_FAILURE);
  }
  
  // move the file pointer to the end to get the file size
  fseek(file_to_send, 0, SEEK_END); // set the pointer to the end
  uint32_t fileSize = ftell(file_to_send); // get the position to determine the size
  fseek(file_to_send, 0, SEEK_SET); // reset the pointer to the begin

  // send the file size
  uint32_t netFileSize = htonl(fileSize);  // convert to network byte order
  if (send(client_socket, &netFileSize, sizeof(netFileSize), 0) == -1) {
    perror("Error sending file size");
    fclose(file_to_send);
    exit(EXIT_FAILURE);
  }
  
  // receive the length of response
  uint32_t response_length;
  bytes_read = recv(client_socket, &response_length, sizeof(response_length), 0);
  if (bytes_read <= 0) {
    perror("Error receiving file path length");
    exit(EXIT_FAILURE);
  }
  response_length = ntohl(response_length); // convert from network byte order

  char *response = malloc(response_length + 1); // memory allocation for receive the response
  if (response == NULL) {
    perror("Error allocating memory");
    exit(EXIT_FAILURE);
  }

  // receive the response from the server
  recv(client_socket, response, response_length, 0);
  if (response_length > 0) {
    if (strcmp(response, "OK") == 0) { // if response is OK
      // read and send the byte stream
      while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file_to_send)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) == -1) {
          perror("Error in send()");
          fclose(file_to_send);
          free(response);
          exit(EXIT_FAILURE);
        }
      }

      // intecept error on reading
      if (bytes_read == -1) {
        perror("Error reading file");
        fclose(file_to_send);
        free(response);
        exit(EXIT_FAILURE);
      }

      printf("File sent successfully.\n");
    } else { // if error in response print it
      printf("%s\n", response);
    } 
  } else { // if no response from the server
    printf("No response from the server.\n");
  }
  
  fclose(file_to_send);
  free(response);
}