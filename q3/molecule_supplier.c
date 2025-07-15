/*
 * תיאור כללי של השרת:
 * --------------------
 * molecule_supplier הוא שרת שמאפשר ניהול מלאי של אטומים ויצירת מולקולות.
 * השרת תומך בשני פרוטוקולים במקביל:
 * 
 * 1. חיבור TCP:
 *    - משמש להוספת אטומים למלאי
 *    - לקוחות יכולים לשלוח פקודות בפורמט: "ADD <סוג האטום> <כמות>"
 *    - סוגי האטומים האפשריים: CARBON, HYDROGEN, OXYGEN
 *    - דוגמה: "ADD CARBON 100"
 * 
 * 2. חיבור UDP:
 *    - משמש לבקשת יצירת מולקולות
 *    - לקוחות יכולים לשלוח פקודות בפורמט: "DELIVER <סוג המולקולה> <כמות>"
 *    - סוגי המולקולות האפשריים:
 *      * WATER (H2O) - דורש 2 מימן ו-1 חמצן לכל יחידה
 *      * CARBON DIOXIDE (CO2) - דורש 1 פחמן ו-2 חמצן לכל יחידה
 *      * ALCOHOL (C2H6O) - דורש 2 פחמן, 6 מימן ו-1 חמצן לכל יחידה
 *      * GLUCOSE (C6H12O6) - דורש 6 פחמן, 12 מימן ו-6 חמצן לכל יחידה
 *    - דוגמה: "DELIVER WATER 10" (ייצור 10 מולקולות מים)
 * 
 * הפעלת השרת:
 * ./molecule_supplier <TCP port> <UDP port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>

#define MAX_CLIENTS 100       // Maximum number of TCP clients that can be connected simultaneously
#define BUFFER_SIZE 1024      // Size of the buffer for receiving data
#define MAX_ATOMS 1000000000000000000ULL  // Maximum number of atoms per type (10^18)


/**
 * Structure to store the current inventory of atoms
 * Using unsigned long long to support the large number requirement (10^18)
 */
typedef struct {
    unsigned long long carbon;    // Count of carbon atoms
    unsigned long long hydrogen;  // Count of hydrogen atoms
    unsigned long long oxygen;    // Count of oxygen atoms
} AtomStock;

// Global variable to store the current atom inventory
AtomStock stock = {0, 0, 0};

// Counter for the number of connected TCP clients
int connected_clients = 0;

/**
 * Prints the current atom inventory to stdout
 * Called after each successful operation to show the updated stock
 */
void print_stock() {
    printf("Stock: C=%llu, H=%llu, O=%llu\n", stock.carbon, stock.hydrogen, stock.oxygen);
}

/**
 * Adds atoms to the stock inventory
 * 
 * @param stock    Pointer to the atom stock structure
 * @param element  Type of atom to add ("CARBON", "HYDROGEN", or "OXYGEN")
 * @param amount   Number of atoms to add
 * @return         1 on success, 0 on failure
 */
int atom_adder(AtomStock *stock, const char *element, unsigned int amount) {
    if (strcmp(element, "CARBON") == 0) {
        // Check if adding would exceed the maximum allowed atoms
        if ((unsigned long long)stock->carbon + amount > MAX_ATOMS) {
            fprintf(stderr, "Error: Exceeds MAX_ATOMS for CARBON\n");
            return 0;
        }
        stock->carbon += amount;
    } else if (strcmp(element, "HYDROGEN") == 0) {
        if ((unsigned long long)stock->hydrogen + amount > MAX_ATOMS) {
            fprintf(stderr, "Error: Exceeds MAX_ATOMS for HYDROGEN\n");
            return 0;
        }
        stock->hydrogen += amount;
    } else if (strcmp(element, "OXYGEN") == 0) {
        if ((unsigned long long)stock->oxygen + amount > MAX_ATOMS) {
            fprintf(stderr, "Error: Exceeds MAX_ATOMS for OXYGEN\n");
            return 0;
        }
        stock->oxygen += amount;
    } else {
        // Invalid atom type
        fprintf(stderr, "Error: Unknown atom type '%s'\n", element);
        return 0;
    }
    return 1;
}
/**
 * Subtracts atoms from the stock to create molecules
 * 
 * @param stock     Pointer to the atom stock structure
 * @param molecule  Type of molecule to create
 * @param amount    Number of molecules to create
 * @return          1 on success, 0 on failure (insufficient atoms or unknown molecule)
 */
int molecule_subtract(AtomStock *stock, const char *molecule, unsigned int amount) {
    unsigned long long need_c = 0, need_h = 0, need_o = 0;
    
    // Calculate required atoms based on molecule type
    if (strcmp(molecule, "WATER") == 0) { // H2O
        need_h = 2 * (unsigned long long)amount;
        need_o = 1 * (unsigned long long)amount;
    } else if (strcmp(molecule, "CARBON DIOXIDE") == 0) { // CO2
        need_c = 1 * (unsigned long long)amount;
        need_o = 2 * (unsigned long long)amount;
    } else if (strcmp(molecule, "ALCOHOL") == 0) { // C2H6O
        need_c = 2 * (unsigned long long)amount;
        need_h = 6 * (unsigned long long)amount;
        need_o = 1 * (unsigned long long)amount;
    } else if (strcmp(molecule, "GLUCOSE") == 0) { // C6H12O6
        need_c = 6 * (unsigned long long)amount;
        need_h = 12 * (unsigned long long)amount;
        need_o = 6 * (unsigned long long)amount;
    } else {
        // Unknown molecule type
        fprintf(stderr, "Error: Unknown molecule type '%s'\n", molecule);
        return 0;
    }
    
    // Check if we have enough atoms
    if (stock->carbon < need_c || stock->hydrogen < need_h || stock->oxygen < need_o) {
        fprintf(stderr, "Error: Not enough atoms for molecule %s\n", molecule);
        return 0;
    }
    
    // Subtract the required atoms from stock
    stock->carbon -= need_c;
    stock->hydrogen -= need_h;
    stock->oxygen -= need_o;
    return 1;
}

/**
 * Process TCP commands from clients (ADD operations)
 * 
 * @param cmd       The command string from the client
 * @param stock     Pointer to the atom stock structure
 * @param client_fd The client socket file descriptor
 * @return          0 on completion
 */
int process_tcp_command(const char *cmd, AtomStock *stock, int client_fd) {
    char op[16], atom[16];
    unsigned int amount;
    
    // Parse the command: ADD <atom type> <amount>
    if (sscanf(cmd, "%15s %15s %u", op, atom, &amount) == 3 && strcmp(op, "ADD") == 0) {
        // Try to add atoms to the stock
        if (atom_adder(stock, atom, amount)) {
            // Success message
            const char *ok_msg = "added to warehouse successfully\n";
            if (send(client_fd, ok_msg, strlen(ok_msg), 0) == -1) {
                perror("send to client failed");
            }
            print_stock();
        } else {
            // Error message if adding fails (exceeds max or unknown atom)
            const char *err_msg = "ERROR: Exceeds MAX_ATOMS\n";
            if (send(client_fd, err_msg, strlen(err_msg), 0) == -1) {
                perror("send to client failed");
            }
        }
    } else {
        // Invalid command format
        fprintf(stderr, "Invalid command from client: %s\n", cmd);
    }
    return 0;
}
/**
 * Process UDP commands from clients (DELIVER operations)
 * 
 * @param cmd         The command string from the client
 * @param stock       Pointer to the atom stock structure
 * @param udp_sock    The UDP socket file descriptor
 * @param client_addr Client address structure for sending responses
 * @param addrlen     Length of the client address structure
 * @return            0 on completion
 */
int process_udp_command(const char *cmd, AtomStock *stock, int udp_sock, struct sockaddr_in *client_addr, socklen_t addrlen) {
    char op[256], molecule1[256], molecule2[256], amount_str[256];
    unsigned int amount;
    char molecule[256];
    
    // Parse the command: DELIVER <molecule> <amount> or DELIVER <molecule1> <molecule2> <amount>
    int n = sscanf(cmd, "%255s %255s %255s %255s", op, molecule1, molecule2, amount_str);

    // Validate command format
    if (n < 3 || strcmp(op, "DELIVER") != 0) {
        fprintf(stderr, "Invalid command from client: %s\n", cmd);
        const char *err_msg = "ERROR: Invalid UDP command\n";
        sendto(udp_sock, err_msg, strlen(err_msg), 0, (struct sockaddr*)client_addr, addrlen);
        return 0;
    }

    // Handle both formats: "DELIVER WATER 5" or "DELIVER CARBON DIOXIDE 10"
    if (n == 4) {
        // Two-word molecule name (e.g., "CARBON DIOXIDE")
        snprintf(molecule, sizeof(molecule), "%s %s", molecule1, molecule2);
    } else {
        // One-word molecule name (e.g., "WATER")
        strncpy(molecule, molecule1, sizeof(molecule));
        molecule[sizeof(molecule)-1] = '\0';
        strncpy(amount_str, molecule2, sizeof(amount_str));
        amount_str[sizeof(amount_str)-1] = '\0';
    }

    // Validate that amount is numeric
    for (int i = 0; amount_str[i]; ++i) {
        if (!isdigit((unsigned char)amount_str[i])) {
            const char *err_msg = "ERROR: Invalid amount\n";
            sendto(udp_sock, err_msg, strlen(err_msg), 0, (struct sockaddr*)client_addr, addrlen);
            return 0;
        }
    }
    
    // Convert amount string to unsigned int
    amount = (unsigned int)strtoul(amount_str, NULL, 10);
    if (amount == 0) {
        const char *err_msg = "ERROR: Amount must be positive\n";
        sendto(udp_sock, err_msg, strlen(err_msg), 0, (struct sockaddr*)client_addr, addrlen);
        return 0;
    }

    // Try to create the molecules
    if (molecule_subtract(stock, molecule, amount)) {
        // Success message
        const char *ok_msg = "Molecule delivered successfully\n";
        if (sendto(udp_sock, ok_msg, strlen(ok_msg), 0, (struct sockaddr*)client_addr, addrlen) == -1) {
            perror("sendto to client failed");
        }
        print_stock();
    } else {
        // Error message if creating molecules fails
        const char *err_msg = "ERROR: Not enough atoms or unknown molecule\n";
        if (sendto(udp_sock, err_msg, strlen(err_msg), 0, (struct sockaddr*)client_addr, addrlen) == -1) {
            perror("sendto to client failed");
        }
    }
    return 0;
}



/**
 * Main function for the molecule supplier server
 * 
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return     Exit code
 */
int main(int argc, char *argv[]) {
    // Validate command line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage %s <TCP port> <UDP port>\n", argv[0]);
        exit(1);
    }

    // Parse and validate TCP port
    int TCP_port = atoi(argv[1]);
    if (TCP_port <= 0 || TCP_port > 65535) {
        fprintf(stderr, "Error: Invalid TCP port number %s\n", argv[1]);
        exit(1);
    }
    
    // Parse and validate UDP port
    int UDP_port = atoi(argv[2]);
    if (UDP_port <= 0 || UDP_port > 65535) {
        fprintf(stderr, "Error: Invalid UDP port number %s\n", argv[2]);
        exit(1); }
    
    int tcp_sock, udp_sock, new_sock;
    struct sockaddr_in tcp_addr, udp_addr;
    char buffer[BUFFER_SIZE];

    // Create TCP socket for atom addition operations
    if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("TCP socket failed");
        exit(1);
    }

    // Create UDP socket for molecule delivery operations
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket failed");
        exit(1);
    }

    // Configure TCP socket address
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all available interfaces
    tcp_addr.sin_port = htons(TCP_port);    // Set the TCP port

    // Configure UDP socket address
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all available interfaces
    udp_addr.sin_port = htons(UDP_port);    // Set the UDP port

    // Bind sockets to their addresses
    if (bind(tcp_sock, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("TCP bind failed");
        exit(1);
    }
    if (bind(udp_sock, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("UDP bind failed");
        exit(1);
    }

    // Set TCP socket to listen mode with a backlog of 10 connections
    if (listen(tcp_sock, 10) < 0) {
        perror("TCP listen failed");
        exit(1);
    }

    printf("Server listening: TCP on port %d, UDP on port %d\n", TCP_port, UDP_port);

    // Initialize file descriptor sets for select()
    fd_set readfds, master_set;
    int maxfd = (tcp_sock > udp_sock ? tcp_sock : udp_sock) + 1;
    FD_ZERO(&master_set);
    FD_SET(tcp_sock, &master_set);    // Add TCP socket to monitoring set
    FD_SET(udp_sock, &master_set);    // Add UDP socket to monitoring set

    // Main server loop
    while (1) {
        // Make a copy of the master set for select()
        readfds = master_set;

        // Wait for activity on any of the sockets
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }

        // Check all file descriptors for activity
        for (int i = 0; i <= maxfd; i++) {
            if (FD_ISSET(i, &readfds)) {
                if (i == tcp_sock) {
                    // New TCP client connection
                    struct sockaddr_in client_addr;
                    socklen_t addrlen = sizeof(client_addr);
                    int new_fd = accept(tcp_sock, (struct sockaddr*)&client_addr, &addrlen);
                    if (new_fd == -1) {
                        perror("accept");
                    } else if (connected_clients >= MAX_CLIENTS) {
                        // Too many clients connected, reject this connection
                        const char *err_msg = "ERROR: Server reached maximum client limit\n";
                        send(new_fd, err_msg, strlen(err_msg), 0);
                        close(new_fd);
                        printf("Connection rejected: maximum clients limit reached\n");
                    } else {
                        // Add new client socket to the monitoring set
                        FD_SET(new_fd, &master_set);
                        if (new_fd > maxfd) maxfd = new_fd;
                        printf("New TCP connection on socket %d\n", new_fd);
                        connected_clients++;
                    }
                } else if (i == udp_sock) {
                    // UDP datagram received
                    struct sockaddr_in client_addr;
                    socklen_t addrlen = sizeof(client_addr);
                    int n = recvfrom(udp_sock, buffer, sizeof(buffer), 0,
                                    (struct sockaddr*)&client_addr, &addrlen);
                    if (n < 0) {
                        perror("recvfrom");
                    } else {
                        // Null-terminate the received data and process it
                        buffer[n] = '\0';
                        process_udp_command(buffer, &stock, i, &client_addr, addrlen);
                    }
                
                } else { // Handle data from TCP clients
                    // Receive data from a TCP client
                    ssize_t n = recv(i, buffer, sizeof(buffer) - 1, 0);
                    if (n <= 0) {
                        // Connection closed or error
                        if (n < 0) perror("recv");
                        printf("Client fd=%d disconnected\n", i);
                        close(i);  // Close the socket
                        FD_CLR(i, &master_set);  // Remove from monitoring set
                        connected_clients--;  // Decrement client counter

                    } else {
                        // Null-terminate the received data and process it
                        buffer[n] = '\0';
                        process_tcp_command(buffer, &stock, i);
                    }
                }
            }
        }
    }


}