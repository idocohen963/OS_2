#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>     
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <getopt.h>
#include <sys/un.h>

#define BUFFER_SIZE 1024

/**
 * Validates if a TCP command is in the correct format: ADD <ATOM> <AMOUNT>
 * 
 * @param command   Command string to validate
 * @return          1 if the command is valid, 0 if not
 */
int validate_tcp_command(const char *command) {
    char add[16], atom_type[16], amount_str[32];
    
    // Split the command into parts
    if (sscanf(command, "%15s %15s %31s", add, atom_type, amount_str) != 3) {
        return 0;
    }
    
    // Verify the command starts with ADD
    if (strcmp(add, "ADD") != 0) {
        return 0;
    }
    
    // Verify the atom type is valid
    if (strcmp(atom_type, "CARBON") != 0 && 
        strcmp(atom_type, "HYDROGEN") != 0 && 
        strcmp(atom_type, "OXYGEN") != 0) {
        return 0;
    }
    
    // Verify the amount is a valid number
    for (int i = 0; amount_str[i]; ++i) {
        if (!isdigit((unsigned char)amount_str[i])) {
            return 0;
        }
    }
    
    // Verify the number is not too big for unsigned int
    if (strlen(amount_str) > 10 || (strlen(amount_str) == 10 && strcmp(amount_str, "4294967295") > 0)) {
        return 0;
    }
    
    unsigned int amount = (unsigned int)strtoul(amount_str, NULL, 10);
    if (amount == 0) {
        return 0; // Amount must be greater than 0
    }
    
    return 1;
}

/**
 * Creates a TCP socket and connects to the server
 * 
 * @param host  Server hostname or IP address
 * @param port  Port number to connect to
 * @return      Connected socket file descriptor, or -1 on error
 */
int setup_tcp_socket(const char *host, const char *port) {
    struct addrinfo hints, *server_info, *p;
    int sock_fd;
    int rv;
    
    // Initialize hints structure
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;     // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    
    // Get address information
    if ((rv = getaddrinfo(host, port, &hints, &server_info)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }
    
    // Loop through all the results and connect to the first we can
    for (p = server_info; p != NULL; p = p->ai_next) {
        if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        
        if (connect(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sock_fd);
            perror("client: connect");
            continue;
        }
        
        break; // If we get here, we successfully connected
    }
    
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return -1;
    }
    
    freeaddrinfo(server_info);
    return sock_fd;
}

/**
 * Creates a Unix Domain Socket and connects to the server
 * 
 * @param socket_path  Path to the UDS file
 * @return             Connected socket file descriptor, or -1 on error
 */
int setup_uds_socket(const char *socket_path) {
    int sock_fd;
    struct sockaddr_un addr;
    
    // Create socket
    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }
    
    // Set up address structure
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    // Connect to server
    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        close(sock_fd);
        return -1;
    }
    
    return sock_fd;
}

/**
 * Main function - creates TCP socket, receives commands from user and sends them to server
 * 
 * @param argc  Number of command line arguments
 * @param argv  Array of command line arguments
 * @return      Exit code
 */
int main(int argc, char *argv[]) {
    
    int opt;
    const char *host = NULL;
    const char *port = NULL;
    const char *socket_path = NULL;
    
    // Process command line options
    while ((opt = getopt(argc, argv, "h:p:f:")) != -1) {
        switch (opt) {
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'f':
                socket_path = optarg;
                break; 
            default:
                fprintf(stderr, "Usage: %s -h <hostname/IP> -p <port> OR %s -f <UDS socket file path>\n", 
                        argv[0], argv[0]);
                exit(1);
        }
    }

    // Check for conflicting arguments
    if ((host != NULL || port != NULL) && socket_path != NULL) {
        fprintf(stderr, "Error: Cannot specify both IP address/port and UDS socket path\n");
        fprintf(stderr, "Usage: %s -h <hostname/IP> -p <port> OR %s -f <UDS socket file path>\n", 
                argv[0], argv[0]);
        exit(1);
    }

    // Check if valid arguments were provided
    if (socket_path == NULL && (host == NULL || port == NULL)) {
        fprintf(stderr, "Usage: %s -h <hostname/IP> -p <port> OR %s -f <UDS socket file path>\n", 
                argv[0], argv[0]);
        exit(1);
    }
    
    // Set up socket based on provided arguments
    int sock_fd;
    
    if (socket_path != NULL) {
        // UDS mode
        sock_fd = setup_uds_socket(socket_path);
        if (sock_fd == -1) {
            exit(EXIT_FAILURE);
        }
        printf("Connected to atom warehouse server using UDS at %s\n", socket_path);
    } else {
        // TCP mode
        sock_fd = setup_tcp_socket(host, port);
        if (sock_fd == -1) {
            exit(EXIT_FAILURE);
        }
        printf("Connected to atom warehouse server at %s:%s\n", host, port);
    }
    
    printf("Enter command: ADD <ATOM_TYPE> <AMOUNT>\n");
    printf("Available atom types: CARBON, HYDROGEN, OXYGEN\n");
    
    char command[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    
    // Rest of the main function remains unchanged
    while (1) {
        printf("Enter command: ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break; // End of file or error
        }
        
        // Remove trailing newline
        command[strcspn(command, "\n")] = 0;
        
        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            printf("Exiting...\n");
            break;
        }
        
        if (validate_tcp_command(command)) {
            // Send command to server
            if (send(sock_fd, command, strlen(command), 0) == -1) {
                perror("send failed");
                break;
            }
            
            // Receive response from server
            int bytes_received = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    printf("Server closed connection\n");
                } else {
                    perror("recv failed");
                }
                break;
            }
            
            // Null-terminate the response and display it
            buffer[bytes_received] = '\0';
            printf("Server response: %s", buffer);
        } else {
            printf("Invalid command format or values.\n");
        }
    }
    
    close(sock_fd);
    return 0;
}
