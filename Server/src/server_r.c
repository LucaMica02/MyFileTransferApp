#include "server.h"

// this function manage the -r command server-side
void server_r(int client_socket, char *curr_path) {
  char *response = "OK";
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
    return;
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

  file_path[bytes_read] = '\0';  // null-terminate the string
  // concatane the file path to the relative root to build the final path
  file_path = concatenate_strings(curr_path, file_path, 1);

  // intercept path error 
  if (stat(file_path, &st) != 0) { // the file doesn't exists
    response = "Error: the file doesn't exists";
    right_path = 0;
  } else if (S_ISDIR(st.st_mode)) { // exists but is a directory
    response = "Error: unable to read file, it is a directory";
    right_path = 0;
  } else if (detect_path_traversal(file_path) == 1) { // path traveresal detected
    response = "Error: path traversal not allowed";
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

  // if the file exists
  if (right_path == 1) {
    wait_until_file_available(); // wait until can open a new file
    int file_descriptor = open(file_path, O_RDONLY); // open the file in reading mode
    if (file_descriptor == -1) {
        perror("Errore nell'apertura del file");
        free(file_path);
        return;
    }

    // set the whole file lock in reading mode
    struct flock fl;
    fl.l_type = F_RDLCK;
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

    // move the file pointer to the end to get the file size
    off_t fileSize = lseek(file_descriptor, 0, SEEK_END);
    if (fileSize == -1) {
      perror("Error seeking end of file");
      close(file_descriptor);
      free(file_path);
      return;
    }
    
    // move the file pointer back to the beginning
    if (lseek(file_descriptor, 0, SEEK_SET) == -1) {
      perror("Error seeking beginning of file");
      close(file_descriptor);
      free(file_path);
      return;
    }

    // send the file size
    uint32_t netFileSize = htonl(fileSize);  // convert to network byte order
    if (send(client_socket, &netFileSize, sizeof(netFileSize), 0) == -1) {
      perror("Error sending file size");
      close(file_descriptor);
      free(file_path);
      return;
    }

    // read the bytes from the file and send it to the client
    while ((bytes_read = read(file_descriptor, buffer, BUFFER_SIZE)) > 0) {
      if (send(client_socket, buffer, bytes_read, 0) == -1) {
      perror("Error in send()");
      close(file_descriptor);
      free(file_path);
      return;
      }
    }

    // catch error in the reading
    if (bytes_read == -1) {
      perror("Error reading file");
      close(file_descriptor);
      free(file_path);
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
    printf("File send successfully.\n");
  }
  
  // if an error occurs in the file path print the error
  if (right_path == 0) {
    printf("%s\n", response);
  }

  free(file_path);
}