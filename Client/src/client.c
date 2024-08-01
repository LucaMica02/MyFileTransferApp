#include "client.h"

int main(int argc, char *argv[]) {
    // check if the number of arguments are right
    if (argc < 8) {
        printf("ERROR too few arguments\n");
        exit(EXIT_FAILURE);
    } else if (argc > 10) {
        printf("ERROR too many arguments\n");
        exit(EXIT_FAILURE);
    } else if (argc == 9) {
        printf("ERROR wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    // variable declarations
    int options[] = {0, 0, 0, 0, 0}; // -w/r/l, -a, -p, -f, -o;
    int client_socket;
    struct sockaddr_in server_address; // for handling Internet addresses in the IPv4 protocol
    char *command_str; 
    char command; // for store the command to pass at the server
    char *address;
    char *port;
    char *file_path_f;
    char *file_path_o;

    // check for the right input format
    for (int i = 1; i < argc; i+=2) { 
        // store the command
        if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "-l") == 0) {
            command_str = argv[i];
            command = command_str[1];
            options[0] = 1;
            i--;
        } else if (strcmp(argv[i], "-a") == 0) {
            options[1] = 1;
            address = argv[i+1];
            if (inet_addr(address) == -1) { // address error
                printf("ERROR wrong address format, follow the ipv4 standard\n");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-p") == 0) {
            options[2] = 1;
            port = argv[i+1];
            if (atoi(port) < 1024 || atoi(port) > 65535) { // port error
                printf("ERROR port number not allowed, it must be between 1024 and 65535\n");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-f") == 0) { // store the -f file
            options[3] = 1;
            file_path_f = argv[i+1];
        } else if (strcmp(argv[i], "-o") == 0) { // store the -o file
            options[4] = 1;
            file_path_o = argv[i+1];
        } else { // wrong input format error
            printf("ERROR wrong input format\n");
            exit(EXIT_FAILURE);
        }
    }

    // check if all options are specified
    if (options[0] == 0) {
        printf("ERROR command missing\n");
        exit(EXIT_FAILURE);
    } else if (options[1] == 0) {
        printf("ERROR address missing\n");
        exit(EXIT_FAILURE);
    } else if (options[2] == 0) {
        printf("ERROR port missing\n");
        exit(EXIT_FAILURE);
    } else if (options[3] == 0) {
        printf("ERROR file path missing\n");
        exit(EXIT_FAILURE);
    } else if (options[4] == 0) {
        file_path_o = file_path_f;
    }

    // check for wrong input format error
    // -l command can't have 10 argc
    // if have 10 argc I have to have -o option
    if ((argc == 10 && command == 'l') || (argc == 10 && options[4] == 0)) {
        printf("ERROR wrong input format\n");
        exit(EXIT_FAILURE);
    }

    // open the client socket with address family IPv4 and a stream socket for TCP
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error in socket()");
        exit(EXIT_FAILURE);
    }
    
    // preparation of the server's local address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(port));
    server_address.sin_addr.s_addr = inet_addr(address);
    
    // connect the client to the server
    if ((connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address))) == -1) {
        perror("Error in connect()");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // set a timeout of 10sec
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    // set timeout on recv to prevent when the server closes the connection unexpectedly
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    
    printf("Client connected to %s\n", address);

    // send the command to the server
    if (send(client_socket, &command, 1, 0) == -1) {
        perror("Error in send()");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // determines which client-side function to call
    // based on the command received in input
    switch (command)
    {
    case 'w':
        client_w(client_socket, file_path_o, file_path_f);
        break;
    case 'r':
        client_r(client_socket, file_path_f, file_path_o);
        break;
    default:
        client_l(client_socket, file_path_f);
        break;
    }

    close(client_socket);
    return 0;
}