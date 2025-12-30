/*
 * תיאור כללי של הלקוח:
 * --------------------
 * atom_supplier הוא לקוח TCP שמתקשר עם שרת atom_warehouse.
 * הלקוח מאפשר למשתמש לשלוח בקשות להוספת אטומים בפורמט הבא:
 * ADD <סוג האטום> <כמות>
 * 
 * סוגי האטומים האפשריים:
 * - CARBON (פחמן)
 * - HYDROGEN (מימן)
 * - OXYGEN (חמצן)
 * 
 * הלקוח שולח את הבקשה לשרת, מחכה לתשובה ומציג אותה למשתמש.
 * 
 * הפעלת הלקוח:
 * ./atom_supplier <hostname/IP> <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>

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
 * Main function - creates TCP socket, receives commands from user and sends them to server
 * 
 * @param argc  Number of command line arguments
 * @param argv  Array of command line arguments
 * @return      Exit code
 */
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <hostname/IP> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    const char *host = argv[1];
    const char *port = argv[2];
    
    // Set up TCP socket and connect to server
    int sock_fd = setup_tcp_socket(host, port);
    if (sock_fd == -1) {
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to atom warehouse server at %s:%s\n", host, port);
    printf("Enter command: ADD <ATOM_TYPE> <AMOUNT>\n");
    printf("Available atom types: CARBON, HYDROGEN, OXYGEN\n");
    
    char command[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    
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
