#include "server.h"
#include <sys/time.h> // for timeval struct

// mutex for manage the thread access to the function for 
// check if there is enough space on the memory
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


// this function manage the comunication with a client
void* handle_client(void *arg) {

    // take the arguments from the struct ThreadArgs and free the memory
    ThreadArgs *args = (ThreadArgs*) arg;
    int client_socket = args->client_socket;
    char *curr_path = args->curr_path;
    free(args);  

    ssize_t bytes_read; // for intercept errors in recv
    char command; // for store the command

    // receive the command from the client and check if there are errors
    bytes_read = recv(client_socket, &command, 1, 0);
    if (bytes_read <= 0) {
        perror("Error receiving command");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // determines which server-side function to call
    // based on the command received from the client
    switch (command)
        {
        case 'w':
            server_w(client_socket, curr_path);
            break;
        case 'r':
            server_r(client_socket, curr_path);
            break;
        default:
            server_l(client_socket, curr_path);
            break;
        }
    
    // close the socket and terminate the thread when is done
    close(client_socket);
    pthread_exit(NULL);
}


// this main function implements a server that accepts client 
// connections and processes their requests in separate threads
int main(int argc, char *argv[]) {

    // print the right format when intercept input error
    char right_format[] = "./myFTserver -a address -p port -f root_directory";

    // check if the number of arguments are right
    if (argc < 7) {
        printf("ERROR too few arguments try: %s\n", right_format);
        exit(EXIT_FAILURE);
    } else if (argc > 7) {
        printf("ERROR too many arguments try: %s\n", right_format);
        exit(EXIT_FAILURE);
    }

    // variables declaration
    int options[] = {0, 0, 0}; // -a, -p, -f
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address; // for handling Internet addresses in the IPv4 protocol
    socklen_t client_address_len; // for manage the size of client address
    char *client_IP; // for store the client address in string format
    char *address; // address of the server
    char *port; // port of the server
    char *root; // relative root of the server
    char *token; // used for the function strtok()
    char *curr_path = ""; // used for check or create the path
    int flag = 0; // used for rightly build the root path

    // check for the right input format
    for (int i = 1; i < 7; i+=2) {
        if (strcmp(argv[i], "-a") == 0) { // address error
            options[0] = 1;
            address = argv[i+1];
            if (inet_addr(address) == -1) {
                printf("ERROR wrong address format, follow the ipv4 standard\n");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-p") == 0) { // port error
            options[1] = 1;
            port = argv[i+1];
            if (atoi(port) < 1024 || atoi(port) > 65535) {
                printf("ERROR port number not allowed, it must be between 1024 and 65535\n");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-f") == 0) { // relative root error
            options[2] = 1;
            root = argv[i+1];
            if (detect_path_traversal(root) == 1) {
                printf("ERROR in root directory: path traversal not allowed\n");
                exit(EXIT_FAILURE);
            }
        } else { // input format error 
            printf("ERROR wrong input format try: %s\n", right_format);
            exit(EXIT_FAILURE);
        }
    }

    // check if all options are specified
    if (options[0] == 0) {
        printf("ERROR address missing try: %s\n", right_format);
        exit(EXIT_FAILURE);
    } else if (options[1] == 0) {
        printf("ERROR port missing try: %s\n", right_format);
        exit(EXIT_FAILURE);
    } else if (options[2] == 0) {
        printf("ERROR root directory missing try: %s\n", right_format);
        exit(EXIT_FAILURE);
    }


    // check if the path exists, else create it
    token = strtok(root, "/"); // split the string root 
    while (token != NULL) {
        curr_path = concatenate_strings(curr_path, token, flag); // build the path
        check_dir(curr_path); // check if the directory exists or create it
        token = strtok(NULL, "/");
        flag++;
    }

    // open the server socket with address family IPv4 and a stream socket for TCP
    if (((server_socket = socket(AF_INET, SOCK_STREAM, 0))) == -1) {
        perror("Error in server socket()");
        exit(EXIT_FAILURE);
    }
        
    // preparation of the server's local address
    server_address.sin_family = AF_INET; // IPv4 family
    server_address.sin_port = htons(atoi(port)); // convert the string to int and then to network byte format
    server_address.sin_addr.s_addr = inet_addr(address); // convert the string in network byte format
    
    // assigns the address specified by server_address to the socket
    // referred to by the file descriptor server_socket
    if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
        perror("Error in server socket bind()");
        exit(EXIT_FAILURE);
    }
    
    // listens to the server with a maximum queue of 512 connections
    if (listen(server_socket, 512) == -1) { // found in /proc/sys/net/ipv4/tcp_max_syn_backlog
        perror("Error in server socket listen()");
        exit(EXIT_FAILURE);
    }

    client_address_len = sizeof(client_address); // take the size of the client address

    // while loop for accept a new connection and create a thread to manage it 
    while (1) {
        usleep(10000); // sleep for 0.01 sec
        wait_until_file_available(); // wait until can open a new file

        // accept the connection and create the client socket
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len)) == -1) {
            perror("Error in accept()");
            continue;
        }

        // set a timeout of 10sec
        struct timeval timeout;
        timeout.tv_sec = 10; 
        timeout.tv_usec = 0;

        // set timeout on recv from client socket to prevent when a client closes the connection unexpectedly
        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
            perror("setsockopt failed");
            close(client_socket);
            continue;
        }

        // store the client's address in string format 
        client_IP = inet_ntoa(client_address.sin_addr);
        printf("\nClient @ %s connects on socket %d\n", client_IP, client_socket);

        // create the struct ThreadArgs and set the information to pass at the thread
        ThreadArgs *args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
        if (args == NULL) {
            perror("Error allocating memory for thread arguments");
            close(client_socket);
            continue;
        }
        args->client_socket = client_socket;
        args->curr_path = curr_path;

        // create a new thread to handle the client connection
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, (void*) args) != 0) {
            perror("Error creating thread");
            free(args);
            close(client_socket);
            continue;
        }

        // detach the thread to handle its own resources
        pthread_detach(thread);
    }


    close(server_socket); // close server connection
    free(curr_path); // free memory allocated for curr_path
    pthread_mutex_destroy(&mutex); // destroy the mutex
    return 0;
}