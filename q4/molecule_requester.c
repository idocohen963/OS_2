#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>  
#include <getopt.h>

#define BUFFER_SIZE 1024

/**
 * Validates if a UDP command is in the correct format: DELIVER <MOLECULE> <AMOUNT>
 * 
 * @param command   Command string to validate
 * @return          1 if the command is valid, 0 if not
 */
int validate_udp_command(const char *command) {
    char deliver[256], molecule1[256], molecule2[256], amount_str[256];
    unsigned int amount;
    int n = sscanf(command, "%255s %255s %255s %255s", deliver, molecule1, molecule2, amount_str);
    if (n < 3) return 0;

    // Check that the command starts with DELIVER
    if (strcmp(deliver, "DELIVER") != 0) return 0;

    // Determine molecule name and amount string
    char molecule[512]; 
    if (n == 4) {
        // Molecule name is two words
        snprintf(molecule, sizeof(molecule), "%s %s", molecule1, molecule2);
        // No need to copy amount_str, it's already in the right variable
    } else if (n == 3) {
        // Molecule name is one word
        strncpy(molecule, molecule1, sizeof(molecule) - 1);
        molecule[sizeof(molecule) - 1] = '\0';
        
        // Copy amount from molecule2 into amount_str
        strncpy(amount_str, molecule2, sizeof(amount_str) - 1);
        amount_str[sizeof(amount_str) - 1] = '\0';
    } else {
        return 0;
    }

    // Check if amount_str contains only digits
    for (int i = 0; amount_str[i]; ++i) {
        if (!isdigit((unsigned char)amount_str[i])) {
            return 0;
        }
    }
    
    // Check if the number is not too big for unsigned int
    if (strlen(amount_str) > 10 || (strlen(amount_str) == 10 && strcmp(amount_str, "4294967295") > 0)) {
        return 0;
    }
    
    amount = (unsigned int)strtoul(amount_str, NULL, 10);

    // Check if molecule is valid and amount is greater than zero
    if ((strcmp(molecule, "WATER") == 0 ||
         strcmp(molecule, "CARBON DIOXIDE") == 0 ||
         strcmp(molecule, "ALCOHOL") == 0 ||
         strcmp(molecule, "GLUCOSE") == 0) &&
        amount > 0) {
        return 1;
    }
    
    return 0;
}

/**
 * Creates a UDP socket and prepares server address
 * 
 * @param host         Server hostname or IP address
 * @param port         Port number to connect to
 * @param server_addr  Pointer to server address structure to be populated
 * @return             Created socket file descriptor
 */
int setup_udp_socket(const char *host, const char *port, struct sockaddr_in *server_addr) {
    struct addrinfo hints = {0};
    struct addrinfo *res, *p;
    int sockfd;
    
    hints.ai_family = AF_INET;    // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP
    
    int rv;
    if ((rv = getaddrinfo(host, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    // Loop through all the results and make a socket
    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            continue;
        }
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "Failed to create socket\n");
        exit(EXIT_FAILURE);
    }
    
    // Copy the server address information
    memcpy(server_addr, p->ai_addr, sizeof(struct sockaddr_in));
    
    freeaddrinfo(res);
    return sockfd;
}

/**
 * Main function - creates UDP socket, receives commands from user and sends them to server
 * 
 * @param argc  Number of command line arguments
 * @param argv  Array of command line arguments
 * @return      Exit code
 */
int main (int argc ,char* argv[]){
    int opt;
    const char *host = NULL;
    const char *port = NULL;

    // Process command line options
    while ((opt = getopt(argc, argv, "h:p:")) != -1) {
        switch (opt) {
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -h <hostname/IP> -p <port>\n", argv[0]);
                exit(1);
        }
    }

    // Check if both host and port were provided
    if (host == NULL || port == NULL) {
        fprintf(stderr, "Usage: %s -h <hostname/IP> -p <port>\n", argv[0]);
        exit(1);
    }

    // Set up UDP socket and server address
    struct sockaddr_in server_addr;
    int sock = setup_udp_socket(host, port, &server_addr);
    
    printf("UDP socket created successfully\n");
    printf("Enter command: DELIVER <MOLECULE> <AMOUNT>\n");
    printf("Examples: DELIVER WATER 10\n");
    printf("Available molecules: WATER, CARBON DIOXIDE, ALCOHOL, GLUCOSE\n");
    
    while (1) {
        char command[256];
        char buffer[BUFFER_SIZE] = {0};
        
        printf("Enter command: ");
        if (fgets(command, sizeof(command), stdin) != NULL) {
            // Remove trailing newline
            command[strcspn(command, "\n")] = 0;

            if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            printf("Exiting...\n");
            break;
            }
            
            if (validate_udp_command(command)) {
                // Send command to server
                if (sendto(sock, command, strlen(command), 0, 
                          (struct sockaddr *)&server_addr, sizeof(server_addr)) != -1) {
                    printf("Request sent to molecule supplier.\n");
                    
                    // Set up for receiving response
                    struct sockaddr_in from_addr;
                    socklen_t from_len = sizeof(from_addr);
                    
                    /* 
                     * Setting a timeout for receiving data from the server
                     * 
                     * The setsockopt function allows setting various socket options.
                     * In this case, we're setting a 5-second timeout for the recvfrom operation.
                     * Without this setting, the recvfrom function would wait indefinitely for a response,
                     * 
                     * Parameters:
                     * sock - socket descriptor
                     * SOL_SOCKET - option level (socket level)
                     * SO_RCVTIMEO - option to set receive timeout
                     * &tv - pointer to the structure containing timeout value (5 seconds)
                     * sizeof(tv) - size of the structure
                     */
                    struct timeval tv;
                    tv.tv_sec = 5;
                    tv.tv_usec = 0;
                    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                        perror("setsockopt failed");
                    }
                    
                    // Receive response
                    ssize_t n = recvfrom(sock, buffer, BUFFER_SIZE-1, 0, 
                                        (struct sockaddr *)&from_addr, &from_len);
                    if (n > 0) {
                        buffer[n] = '\0';
                        printf("Server response: %s\n", buffer);
                    } else {
                        perror("recvfrom failed or timeout occurred");
                    }
                } else {
                    perror("sendto failed");
                }
            } else {
                printf("Invalid command format or values.\n");
                printf("Valid format: DELIVER <MOLECULE> <AMOUNT>\n");
                printf("Available molecules: WATER, CARBON DIOXIDE, ALCOHOL, GLUCOSE\n");
            }
        }
    }
    
    close(sock);
    return 0;
}




