#include "server.h"

// this function manage the -l command server-side
void server_l(int client_socket, char *curr_path) {

  // variable declarations
  char *response = "OK";
  char buffer[BUFFER_SIZE];
  int right_path = 1; 
  ssize_t bytes_read;
  FILE *cmd;
  struct stat st; // to get information about the file path

  // receive the length of the file path
  uint32_t path_length;
  bytes_read = recv(client_socket, &path_length, sizeof(path_length), 0);
  if (bytes_read <= 0) {
    perror("Error receiving file path length");
    return;
  }
  path_length = ntohl(path_length); // convert from network byte order

  char *path = malloc(path_length + 1); // memory allocation for receive the file path
  if (path == NULL) {
    perror("Error allocating memory");
    return;
  }

  // receive the path from the client
  bytes_read = recv(client_socket, path, path_length, 0);
  if (bytes_read <= 0) {
    perror("Error receiving file path");
    free(path);
    return;
  }

  path[bytes_read] = '\0';  // null-terminate the string

  // build the path concatenating the file path to the relative root
  if (bytes_read == 1) {
    path = concatenate_strings(curr_path, path, 0);
  } else {
    path = concatenate_strings(curr_path, path, 1);

    // intercept path error
    if (stat(path, &st) != 0) { // the path doesn't exists
      response = "Error: the path doesn't exists";
      right_path = 0;
    } else if (detect_path_traversal(path) == 1) { // intercept path traversal
      response = "Error: path traversal not allowed";
      right_path = 0;
    }
  }
    
  // Send the length of the response to the client
  uint32_t response_length = htonl(strlen(response)); // Convert to network byte order
  if (send(client_socket, &response_length, sizeof(response_length), 0) == -1) {
    perror("Error in send()");
    free(path);
    return;
  }

  // Send the response to the client
  if (send(client_socket, response, strlen(response), 0) == -1) {
    perror("Error in send()");
    free(path);
    return;
  }

  // if the path exists and if there isn't path traversal
  if (right_path == 1) {

    // build the ls -la command for the specified path
    char cmd_path[strlen(path) + 8];
    strncpy(cmd_path, "ls -la ", 8); 
    //cmd_path[7] = '\0';
    strncat(cmd_path, path, strlen(path));
    cmd_path[strlen(path) + 7] = '\0';

    wait_until_file_available(); // wait until can open a new file
    cmd = popen(cmd_path, "r"); // execute the command specified in cmd_path
    if (cmd == NULL) {
      perror("popen");            
      free(path);
      return;
    }

    // read the output line by line
    while (fgets(buffer, sizeof(buffer), cmd) != NULL) {

      // send the line to the client
      if (send(client_socket, buffer, sizeof(buffer), 0) == -1) {
        perror("Error in send()");
        free(path);
        pclose(cmd);
        return;
      }
    }

    printf("Command execute successfully.\n");
    pclose(cmd); // close the file
  }
    
  // if an error occurs in the file path print the error
  if (right_path == 0) {
    printf("%s\n", response);
  }

  // free memory
  free(path);
}