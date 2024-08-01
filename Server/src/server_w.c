#include "server.h"

// this function manage the -w command server-side
void server_w(int client_socket, char *curr_path) {
  char *response = "OK";
  char *path_copy;
  char *last_symb; // used for find the last occurency of file path
  char buffer[BUFFER_SIZE];
  int right_path = 1; 
  int sleeps = 0; // for detect a deadlock
  ssize_t bytes_read;
  struct stat st; // to get information about the file path

  // receive the length of the file path
  uint32_t path_length;
  bytes_read = recv(client_socket, &path_length, sizeof(path_length), 0);
  if (bytes_read <= 0) {
    perror("Error receiving file path length");
    close(client_socket);
  }
  path_length = ntohl(path_length); // convert from network byte order

  char *file_path = malloc(path_length + 1); // memory allocation for receive the file path
  if (file_path == NULL) {
    perror("Error allocating memory");
    return;
  }

  // receive the file path from the client
  bytes_read = recv(client_socket, file_path, path_length, 0);
  if (bytes_read <= 0) {
    perror("Error receiving file path");
    free(file_path);
    return;
  }

  // receive file length
  uint32_t netFileSize;
  if (recv(client_socket, &netFileSize, sizeof(netFileSize), 0) <= 0) {
    perror("Error receiving file size");
    free(file_path);
    return;
  }
  uint32_t fileSize = ntohl(netFileSize); // convert from network byte order

  file_path[bytes_read] = '\0';  // null-terminate the string
  file_path = concatenate_strings(curr_path, file_path, 1); // concatane the file path to the relative root

  // check if the path exists
  path_copy = strdup(file_path); // copy the file path so as not to corrupt the original string
  last_symb = strrchr(path_copy, '/'); // find the last occurrency of '/'
  if (last_symb != NULL) {
    *last_symb = '\0'; // if there is truncate the string to check if the path exists
    if (stat(path_copy, &st) != 0 || !(S_ISDIR(st.st_mode))) {
      response = "Error: the path doesn't exists";
      right_path = 0;
    } 
  }

  free(path_copy); // free memory

  // intercept file error 
  if (stat(file_path, &st) == 0 && S_ISDIR(st.st_mode)) { // exists but is a directory
    response = "Error: unable to write file, it is a directory";
    right_path = 0;
  } else if (detect_path_traversal(file_path) == 1) { // path traversal detected
    response = "Error: path traversal not allowed";
    right_path = 0;
  } else if (fileSize > MAX_FILE_SIZE) { // file too large
    response = "Error: file too large, max size allowed 1Gb";
    right_path = 0;
  } else if (not_enough_space(curr_path, fileSize) == 1) { // no enough space
    response = "Error: no enough space on the server";
    right_path = 0;
  }


  // send the length of the response to the client
  uint32_t response_length = htonl(strlen(response)); // convert to network byte order
  if (send(client_socket, &response_length, sizeof(response_length), 0) == -1) {
    perror("Error in send()");
    free(file_path);
    return;
  }

  // send the response to the client
  if (send(client_socket, response, strlen(response), 0) == -1) {
    perror("Error in send()");
    free(file_path);
    return;
  }

  // if not file or path error 
  if (right_path == 1) {
    wait_until_file_available(); // wait until can open a new file
    // open the file or create it in writing mode
    int file_descriptor = open(file_path, O_WRONLY | O_CREAT, 0700); // read, write and execute permissions to the owner
    if (file_descriptor == -1) {
        perror("Errore nell'apertura del file");
        free(file_path);
        return;
    }

    // set the whole file lock in writing mode
    struct flock fl;
    fl.l_type = F_WRLCK; 
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    // set the lock at the file descriptor in non blocking mode
    while (fcntl(file_descriptor, F_SETLK, &fl) == -1) {
      if (sleeps > 10) { // detect deadlock
        free(file_path);
        close(file_descriptor);
        return;
      }
      usleep(rand() % 1500000); // MAX 1.5sec
      sleeps++;
    }
    sleeps = 0; // reset sleeps to 0

    // truncates the file to zero bytes to avoid corrupted files
    if (ftruncate(file_descriptor, 0) == -1) {
        perror("Error truncating file");
        free(file_path);
        close(file_descriptor);
        return;
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
      free(file_path);
      close(file_descriptor);
      return;
    }

    // unlock file in not blocking mode
    fl.l_type = F_UNLCK;
    while (fcntl(file_descriptor, F_SETLK, &fl) == -1) {
      if (sleeps > 10) { // detect deadlock
        free(file_path);
        close(file_descriptor);
        return;
      }
      usleep(rand() % 1500000); // MAX 1.5sec
      sleeps++;
    }

    close(file_descriptor);
    printf("File received successfully.\n");

  }
    
  // if an error occurs in the file path print the error
  if (right_path == 0) {
    printf("%s\n", response);
  }

  free(file_path);
}