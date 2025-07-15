/*
 * תיאור כללי של השרת:
 * --------------------
 * drinks_bar הוא שרת שמאפשר ניהול מלאי של אטומים ויצירת מולקולות ומשקאות.
 * השרת תומך בשלושה מקורות קלט במקביל:
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
 * 3. קונסול (מקלדת):
 *    - משמש לחישוב כמות המשקאות שניתן לייצר
 *    - פקודות אפשריות:
 *      * GEN SOFT DRINK - חישוב כמות משקאות קלים
 *      * GEN VODKA - חישוב כמות וודקה
 *      * GEN CHAMPAGNE - חישוב כמות שמפניה
 *    - מתכונים:
 *      * SOFT DRINK: מים + פחמן דו חמצני + גלוקוז (H2O + CO2 + C6H12O6)
 *      * VODKA: מים + אלכוהול + גלוקוז (H2O + C2H6O + C6H12O6)
 *      * CHAMPAGNE: מים + פחמן דו חמצני + אלכוהול (H2O + CO2 + C2H6O)
 * 
 * שלב 6 - מקבוליות תהליכים (Process Concurrency):
 * ----------------------------------------------
 * השרת תומך בריצה מקבילית של מספר תהליכים עם שמירת מצב משותף.
 * 
 * אסטרטגיית המימוש:
 * 1. שמירת מצב בקובץ עם אופציה -f/--save-file
 * 2. מיפוי הקובץ לזיכרון באמצעות mmap() לגישה מהירה ויעילה
 * 3. הגנה על קונקרנטיות באמצעות נעילת קבצים (flock):
 *    - LOCK_EX (Exclusive) לפעולות כתיבה (הוספה/הפחתה של אטומים)
 *    - LOCK_SH (Shared) לפעולות קריאה (הצגת מלאי/חישוב משקאות)
 * 4. מצביע דינמי (stock_ptr) שמצביע על המלאי הפעיל:
 *    - אם אין קובץ שמירה: מצביע על מבנה בזיכרון רגיל
 *    - אם יש קובץ שמירה: מצביע על אזור הזיכרון הממופה
 * 
 * התנהגות הקובץ:
 * - אם הקובץ קיים: טעינת המלאי הקיים והתעלמות מערכי שורת הפקודה
 * - אם הקובץ חדש: יצירה ואתחול עם ערכי שורת הפקודה
 * - אם לא סופק קובץ: התנהגות רגילה (מלאי בזיכרון בלבד)
 * 
 * יתרונות המימוש:
 * - שיתוף מלאי בין תהליכים מרובים
 * - עמידות נתונים (המלאי נשמר גם אחרי סגירת השרת)
 * - ביצועים גבוהים (mmap מהיר יותר מקריאה/כתיבה רגילה)
 * - בטיחות קונקרנטיות (מניעת מצבי race condition)
 * 
 * Server Execution:
 * ./drinks_bar (-T <tcp-port> -U <udp-port>) OR (-s <UDS-stream-path> -d <UDS-datagram-path>) 
 *              [--oxygen N] [--carbon N] [--hydrogen N] [--timeout SECS] [-f <save-file>]
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
#include <getopt.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>


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

/**
 * In-memory stock for when no save-file is provided (preserves original Q5 behavior)
 * This serves as the fallback storage and also holds initial values from command line
 */
AtomStock in_memory_stock = {0, 0, 0};

/**
 * Pointer to the active stock structure
 * By default, points to the in-memory stock for backward compatibility
 * When a save-file is used, this will point to the memory-mapped region
 * All stock operations use this pointer for unified access
 */
AtomStock *stock_ptr = &in_memory_stock;

/**
 * Global file descriptor for file locking to ensure concurrency safety
 * Used with flock() to coordinate access between multiple server processes:
 * - LOCK_EX for exclusive access during write operations
 * - LOCK_SH for shared access during read operations
 * - LOCK_UN to release locks
 */
int lock_fd = -1;

/**
 * Path to the save file for cleanup operations
 * Stored globally to enable cleanup in signal handlers
 */
char* save_file_path = NULL;

// Counter for the number of connected TCP clients
int connected_clients = 0;

// Global variables to track UDS paths for signal handler cleanup
char *global_stream_path = NULL;
char *global_datagram_path = NULL;

/**
 * Signal handler for SIGALRM
 * This function is called when the alarm timer expires (timeout functionality)
 * Performs cleanup operations and gracefully shuts down the server
 * 
 * @param signum The signal number (SIGALRM in this case)
 */
void handle_alarm(int signum) {
    printf("Timeout reached with no activity. Server shutting down.\n");
    
    // Clean up UDS socket files before exit
    if (global_stream_path != NULL) unlink(global_stream_path);
    if (global_datagram_path != NULL) unlink(global_datagram_path);
    
    // Close the file descriptor used for locking (Q6 addition)
    // This automatically releases any locks held by this process
    if (lock_fd != -1) close(lock_fd);
    
    exit(0);
}

/**
 * Prints the current atom inventory to stdout
 * Called after each successful operation to show the updated stock
 * Uses file locking to prevent reading inconsistent data
 * during concurrent write operations by other processes
 */
void print_stock() {
    // Acquire a shared lock for reading to prevent dirty reads
    // Multiple processes can hold shared locks simultaneously for reading
    if (lock_fd != -1) flock(lock_fd, LOCK_SH);

    printf("Stock: C=%llu, H=%llu, O=%llu\n", stock_ptr->carbon, stock_ptr->hydrogen, stock_ptr->oxygen);
    
    // Release the lock to allow other processes to access the file
    if (lock_fd != -1) flock(lock_fd, LOCK_UN);
}

/**
 * Adds atoms to the stock inventory
 * 
 * @param stock    Pointer to the atom stock structure (can be memory-mapped)
 * @param element  Type of atom to add ("CARBON", "HYDROGEN", or "OXYGEN")
 * @param amount   Number of atoms to add
 * @return         1 on success, 0 on failure
 */
int atom_adder(AtomStock *stock, const char *element, unsigned int amount) {
    // Acquire an exclusive lock for writing
    // Only one process can hold an exclusive lock at a time
    if (lock_fd != -1) flock(lock_fd, LOCK_EX);

    int success = 1;
    
    if (strcmp(element, "CARBON") == 0) {
        // Check if adding would exceed the maximum allowed atoms
        if ((unsigned long long)stock->carbon + amount > MAX_ATOMS) {
            fprintf(stderr, "Error: Exceeds MAX_ATOMS for CARBON\n");
            success = 0;
        } else {
            stock->carbon += amount;
        }
    } else if (strcmp(element, "HYDROGEN") == 0) {
        if ((unsigned long long)stock->hydrogen + amount > MAX_ATOMS) {
            fprintf(stderr, "Error: Exceeds MAX_ATOMS for HYDROGEN\n");
            success = 0;
        } else {
            stock->hydrogen += amount;
        }
    } else if (strcmp(element, "OXYGEN") == 0) {
        if ((unsigned long long)stock->oxygen + amount > MAX_ATOMS) {
            fprintf(stderr, "Error: Exceeds MAX_ATOMS for OXYGEN\n");
            success = 0;
        } else {
            stock->oxygen += amount;
        }
    } else {
        // Invalid atom type
        fprintf(stderr, "Error: Unknown atom type '%s'\n", element);
        success = 0;
    }

    // Release the lock before returning to allow other processes to access
    if (lock_fd != -1) flock(lock_fd, LOCK_UN);
    return success;
}

/**
 * Subtracts atoms from the stock to create molecules
 * 
 * @param stock     Pointer to the atom stock structure (can be memory-mapped)
 * @param molecule  Type of molecule to create
 * @param amount    Number of molecules to create
 * @return          1 on success, 0 on failure (insufficient atoms or unknown molecule)
 */
int molecule_subtract(AtomStock *stock, const char *molecule, unsigned int amount) {
    // Acquire an exclusive lock for writing
    // This ensures atomic read-check-write operations across processes
    if (lock_fd != -1) flock(lock_fd, LOCK_EX);

    unsigned long long need_c = 0, need_h = 0, need_o = 0;
    int success = 1;

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
        success = 0;
    }

    if (success) {
        // Check if we have enough atoms (atomic check under lock)
        if (stock->carbon < need_c || stock->hydrogen < need_h || stock->oxygen < need_o) {
            fprintf(stderr, "Error: Not enough atoms for molecule %s\n", molecule);
            success = 0;
        } else {
            // Subtract the required atoms from stock (atomic operation)
            stock->carbon -= need_c;
            stock->hydrogen -= need_h;
            stock->oxygen -= need_o;
        }
    }

    // Release the lock before returning to allow other processes to access
    if (lock_fd != -1) flock(lock_fd, LOCK_UN);
    return success;
}

/**
 * Calculates the maximum number of drinks that can be produced from the current inventory
 * This function computes the minimum between the required molecules for the recipe
 * and what is actually available in the inventory, without modifying the inventory itself
 * Uses shared locking to ensure consistent reads during concurrent operations
 * 
 * @param stock       Pointer to the atom stock structure (can be memory-mapped)
 * @param drink_type  Type of drink to calculate ("SOFT DRINK", "VODKA", or "CHAMPAGNE")
 * @return            Maximum number of drinks that can be produced
 */
unsigned long long calculate_drink_production(AtomStock *stock, const char *drink_type) {
    // Acquire a shared lock for reading to ensure consistent snapshot of stock
    // Multiple processes can read simultaneously but writes are blocked
    if (lock_fd != -1) flock(lock_fd, LOCK_SH);
    
    unsigned long long total_c_needed = 0, total_h_needed = 0, total_o_needed = 0;

    // Calculate total atoms needed per drink for each drink type
    if (strcmp(drink_type, "SOFT DRINK") == 0) {
        // SOFT DRINK: H2O + CO2 + C6H12O6
        // H2O: 2H + 1O
        // CO2: 1C + 2O  
        // C6H12O6: 6C + 12H + 6O
        // Total per drink: 7C + 14H + 9O
        total_c_needed = 7; 
        total_h_needed = 14; 
        total_o_needed = 9;
    } else if (strcmp(drink_type, "VODKA") == 0) {
        // VODKA: H2O + C2H6O + C6H12O6
        // H2O: 2H + 1O
        // C2H6O: 2C + 6H + 1O
        // C6H12O6: 6C + 12H + 6O
        // Total per drink: 8C + 20H + 8O
        total_c_needed = 8; 
        total_h_needed = 20; 
        total_o_needed = 8;
    } else if (strcmp(drink_type, "CHAMPAGNE") == 0) {
        // CHAMPAGNE: H2O + CO2 + C2H6O
        // H2O: 2H + 1O
        // CO2: 1C + 2O
        // C2H6O: 2C + 6H + 1O
        // Total per drink: 3C + 8H + 4O
        total_c_needed = 3; 
        total_h_needed = 8; 
        total_o_needed = 4;
    } else {
        // Unknown drink type - release lock before returning
        if (lock_fd != -1) flock(lock_fd, LOCK_UN);
        return 0;
    }

    // Calculate maximum drinks based on available atoms
    unsigned long long max_drinks = MAX_ATOMS;
    
    if (total_c_needed > 0) {
        unsigned long long drinks_by_carbon = stock->carbon / total_c_needed;
        if (drinks_by_carbon < max_drinks) max_drinks = drinks_by_carbon;
    }
    
    if (total_h_needed > 0) {
        unsigned long long drinks_by_hydrogen = stock->hydrogen / total_h_needed;
        if (drinks_by_hydrogen < max_drinks) max_drinks = drinks_by_hydrogen;
    }
    
    if (total_o_needed > 0) {
        unsigned long long drinks_by_oxygen = stock->oxygen / total_o_needed;
        if (drinks_by_oxygen < max_drinks) max_drinks = drinks_by_oxygen;
    }

    // Release the lock before returning
    if (lock_fd != -1) flock(lock_fd, LOCK_UN);
    
    return (max_drinks == MAX_ATOMS) ? 0 : max_drinks;
}

/**
 * Process commands from console input (GEN operations)
 * 
 * @param cmd    The command string from the console
 * @param stock  Pointer to the atom stock structure (memory or memory-mapped)
 * @return       1 on success, 0 on failure or invalid command
 */
int process_console_command(const char *cmd, AtomStock *stock) {
    char op[256], drink_type1[256], drink_type2[256];

    // Parse command: GEN <DRINK_TYPE> or GEN <DRINK_TYPE1> <DRINK_TYPE2>
    int n = sscanf(cmd, "%255s %255s %255s", op, drink_type1, drink_type2);
    
    if (n < 2 || strcmp(op, "GEN") != 0) {
        printf("Error: Invalid console command. Use: GEN SOFT DRINK / GEN VODKA / GEN CHAMPAGNE\n");
        return 0;
    }
    
    char full_drink_type[512];
    if (n == 3) {
        // Two words (e.g., "SOFT DRINK")
        snprintf(full_drink_type, sizeof(full_drink_type), "%s %s", drink_type1, drink_type2);
    } else {
        // One word (e.g., "VODKA", "CHAMPAGNE")
        strncpy(full_drink_type, drink_type1, sizeof(full_drink_type));
        full_drink_type[sizeof(full_drink_type)-1] = '\0';
    }
    
    // Calculate and display the number of drinks that can be produced
    // Note: calculate_drink_production handles locking internally
    unsigned long long drinks_possible = calculate_drink_production(stock, full_drink_type);
    
    if (strcmp(full_drink_type, "SOFT DRINK") == 0 || 
        strcmp(full_drink_type, "VODKA") == 0 || 
        strcmp(full_drink_type, "CHAMPAGNE") == 0) {
        printf("Can produce %llu %s drinks\n", drinks_possible, full_drink_type);
    } else {
        printf("Error: Unknown drink type '%s'\n", full_drink_type);
    }
    
    return 1;
}

/**
 * Process TCP commands from clients (ADD operations)
 * 
 * @param cmd       The command string from the client
 * @param stock     Pointer to the atom stock structure (memory or memory-mapped)
 * @param client_fd The client socket file descriptor
 * @return          0 on completion
 */
int process_tcp_command(const char *cmd, AtomStock *stock, int client_fd) {
    char op[16], atom[16];
    unsigned int amount;
    
    // Parse the command: ADD <atom type> <amount>
    if (sscanf(cmd, "%15s %15s %u", op, atom, &amount) == 3 && strcmp(op, "ADD") == 0) {
        // Try to add atoms to the stock
        // Note: atom_adder handles locking internally
        if (atom_adder(stock, atom, amount)) {
            // Success message
            const char *ok_msg = "added to warehouse successfully\n";
            if (send(client_fd, ok_msg, strlen(ok_msg), 0) == -1) {
                perror("send to client failed");
            }
            // Note: print_stock handles locking internally
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
 * @param stock       Pointer to the atom stock structure (memory or memory-mapped)
 * @param udp_sock    The UDP socket file descriptor
 * @param client_addr Client address structure for sending responses
 * @param addrlen     Length of the client address structure
 * @return            0 on completion
 */
int process_udp_command(const char *cmd, AtomStock *stock, int udp_sock, struct sockaddr *client_addr, socklen_t addrlen) {
    char op[256], molecule1[256], molecule2[256], amount_str[256];
    unsigned int amount;
    char molecule[512];
    
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
            sendto(udp_sock, err_msg, strlen(err_msg), 0, client_addr, addrlen);
            return 0;
        }
    }
    
    // Convert amount string to unsigned int
    amount = (unsigned int)strtoul(amount_str, NULL, 10);
    if (amount == 0) {
        const char *err_msg = "ERROR: Amount must be positive\n";
        sendto(udp_sock, err_msg, strlen(err_msg), 0, client_addr, addrlen);
        return 0;
    }

    // Try to create the molecules
    // Note: molecule_subtract handles locking internally
    if (molecule_subtract(stock, molecule, amount)) {
        // Success message
        const char *ok_msg = "Molecule delivered successfully\n";
        if (sendto(udp_sock, ok_msg, strlen(ok_msg), 0, client_addr, addrlen) == -1) {
            perror("sendto to client failed");
        }
        // Note: print_stock handles locking internally
        print_stock();
    } else {
        // Error message if creating molecules fails
        const char *err_msg = "ERROR: Not enough atoms or unknown molecule\n";
        if (sendto(udp_sock, err_msg, strlen(err_msg), 0, client_addr, addrlen) == -1) {
            perror("sendto to client failed");
        }
    }
    return 0;
}


/**
 * Main function for the drinks bar server
 * 
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return     Exit code
 */
int main(int argc, char *argv[]) {
    int opt, timeout = 0, UDP_port = -1, TCP_port = -1;
    char *stream_path = NULL, *datagram_path = NULL;
    // save_file_path is declared globally for cleanup access

    static struct option long_options[] = {
        {"oxygen",       required_argument, 0, 'o'},
        {"carbon",       required_argument, 0, 'c'},
        {"hydrogen",     required_argument, 0, 'h'},
        {"timeout",      required_argument, 0, 't'},
        {"tcp-port",     required_argument, 0, 'T'},
        {"udp-port",     required_argument, 0, 'U'},
        {"stream-path",  required_argument, 0, 's'},
        {"datagram-path",required_argument, 0, 'd'},
        {"save-file",    required_argument, 0, 'f'}, 
        {0, 0, 0, 0}
    };

    // Parse command line arguments
    // Note: Initial stock values are stored in in_memory_stock first
    // If a save file is used, we might overwrite these or use them to initialize a new file
    while ((opt = getopt_long(argc, argv, "o:c:h:t:T:U:s:d:f:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'o':
            {
                char *endptr;
                unsigned long long value = strtoull(optarg, &endptr, 10);
                
                // Check for conversion errors
                if (*endptr != '\0' || endptr == optarg || value > MAX_ATOMS) {
                    fprintf(stderr, "Error: Invalid value for oxygen\n");
                    exit(1);
                }
                in_memory_stock.oxygen = value;
                break;
            }
            case 'c':
            {
                char *endptr;
                unsigned long long value = strtoull(optarg, &endptr, 10);
                
                // Check for conversion errors
                if (*endptr != '\0' || endptr == optarg || value > MAX_ATOMS) {
                    fprintf(stderr, "Error: Invalid value for carbon\n");
                    exit(1);
                }
                in_memory_stock.carbon = value;
                break;
            }
            case 'h':
            {
                char *endptr;
                unsigned long long value = strtoull(optarg, &endptr, 10);
                
                // Check for conversion errors
                if (*endptr != '\0' || endptr == optarg || value > MAX_ATOMS) {
                    fprintf(stderr, "Error: Invalid value for hydrogen\n");
                    exit(1);
                }
                in_memory_stock.hydrogen = value;
                break;
            }
            case 't':
                {
                    long v = strtol(optarg, NULL, 10);
                    if(v <= 0) {
                        fprintf(stderr, "invalid timeout\n");
                        exit(1);
                    }
                    timeout = v;
                    break;
                }
            case 'T':
                {
                    long v = strtol(optarg, NULL, 10);
                    if(v <= 0 || v > 65535) {
                        fprintf(stderr, "invalid port\n");
                        exit(1);
                    }
                    TCP_port = v;
                    break;
                }
            case 'U':
                {
                    long v = strtol(optarg, NULL, 10);
                    if(v <= 0 || v > 65535) {
                        fprintf(stderr, "invalid port\n");
                        exit(1);
                    }
                    UDP_port = v;
                    break;
                }
            case 's':
                stream_path = optarg;
                break;
            case 'd':
                datagram_path = optarg;
                break;
            case 'f': 
                save_file_path = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s (-T <tcp-port> -U <udp-port>) OR (-s <UDS-stream-path> -d <UDS-datagram-path>) [--oxygen N] [--carbon N] [--hydrogen N] [--timeout SECS] [-f <save-file>]\n", argv[0]);
                fprintf(stderr, "Note: You must specify either BOTH TCP and UDP ports OR BOTH UDS stream and datagram paths\n");
                exit(1);
        }
    }

   
    if (save_file_path != NULL) {
        printf("Using save file: %s\n", save_file_path);
        
        // Open the file for read/write, create it if it doesn't exist
        // The file descriptor will be used for both mmap and flock operations
        lock_fd = open(save_file_path, O_RDWR | O_CREAT, 0666);
        if (lock_fd == -1) {
            perror("Failed to open or create save file");
            exit(1);
        }

        // Get file status to check if it's a new or existing file
        struct stat file_stat;
        if (fstat(lock_fd, &file_stat) == -1) {
            perror("fstat");
            close(lock_fd);
            exit(1);
        }

        // Check if the file is new (size is 0)
        if (file_stat.st_size == 0) {
            printf("Save file is new. Initializing with provided stock values.\n");
            
            // Expand the new file to the size of our struct
            if (ftruncate(lock_fd, sizeof(AtomStock)) == -1) {
                perror("ftruncate");
                close(lock_fd);
                exit(1);
            }

            // Map the new file to memory with read/write permissions
            // MAP_SHARED ensures changes are visible to other processes
            stock_ptr = mmap(NULL, sizeof(AtomStock), PROT_READ | PROT_WRITE, MAP_SHARED, lock_fd, 0);
            if (stock_ptr == MAP_FAILED) {
                perror("mmap failed on new file");
                close(lock_fd);
                exit(1);
            }
            
            // Initialize the memory-mapped stock with values from command line (or defaults)
            *stock_ptr = in_memory_stock;

        } else {
            printf("Loading stock from existing save file. Ignoring command-line stock values.\n");
            
            // Map the existing file to memory
            stock_ptr = mmap(NULL, sizeof(AtomStock), PROT_READ | PROT_WRITE, MAP_SHARED, lock_fd, 0);
            if (stock_ptr == MAP_FAILED) {
                perror("mmap failed on existing file");
                close(lock_fd);
                exit(1);
            }
        }
    }

    
    // Check that either both TCP and UDP are provided OR both UDS stream and datagram are provided
    if (!((TCP_port != -1 && UDP_port != -1) || 
          (stream_path != NULL && datagram_path != NULL))) {
        
        perror("Connection configuration error");
        fprintf(stderr, "Must specify either both TCP and UDP ports (-T and -U) or both UDS paths (-s and -d)\n");
        fprintf(stderr, "Usage: %s -T <tcp-port> -U <udp-port> OR %s -s <UDS-stream-path> -d <UDS-datagram-path> [--oxygen N] [--carbon N] [--hydrogen N] [--timeout SECS] [-f <save-file>]\n", argv[0], argv[0]);
        exit(1);
    }

    
    int tcp_sock = -1, udp_sock = -1, uds_stream_sock = -1, uds_dgram_sock = -1;
    struct sockaddr_in tcp_addr, udp_addr;
    struct sockaddr_un uds_stream_addr, uds_dgram_addr;
    char buffer[BUFFER_SIZE];

    // TCP/UDP mode
    if (TCP_port != -1 && UDP_port != -1) {
        // Create and configure TCP socket
        if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("TCP socket creation failed");
            exit(1);
        }
        
        // Configure TCP socket address
        memset(&tcp_addr, 0, sizeof(tcp_addr));
        tcp_addr.sin_family = AF_INET;
        tcp_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all available interfaces
        tcp_addr.sin_port = htons(TCP_port);    // Set the TCP port
        
        // Bind TCP socket
        if (bind(tcp_sock, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
            perror("TCP bind failed");
            // Clean up resources before exit
            close(tcp_sock);
            exit(1);
        }
        
        // Create and configure UDP socket
        if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("UDP socket creation failed");
            // Clean up resources before exit
            close(tcp_sock);
            exit(1);
        }
        
        // Configure UDP socket address
        memset(&udp_addr, 0, sizeof(udp_addr));
        udp_addr.sin_family = AF_INET;
        udp_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all available interfaces
        udp_addr.sin_port = htons(UDP_port);    // Set the UDP port
        
        // Bind UDP socket
        if (bind(udp_sock, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
            perror("UDP bind failed");
            // Clean up resources before exit
            close(tcp_sock);
            close(udp_sock);
            exit(1);
        }
        
        printf("TCP/UDP mode initialized successfully\n");
    }
    // UDS mode
    else if (stream_path != NULL && datagram_path != NULL) {
        // Create and configure UDS stream socket
        if ((uds_stream_sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            perror("UDS stream socket creation failed");
            exit(1);
        }
        
        // Configure UDS stream socket address
        memset(&uds_stream_addr, 0, sizeof(uds_stream_addr));
        uds_stream_addr.sun_family = AF_UNIX;
        strncpy(uds_stream_addr.sun_path, stream_path, sizeof(uds_stream_addr.sun_path) - 1);
        
        // Remove existing socket file if present
        unlink(stream_path);
        
        // Bind UDS stream socket
        if (bind(uds_stream_sock, (struct sockaddr*)&uds_stream_addr, sizeof(uds_stream_addr)) < 0) {
            perror("UDS stream bind failed");
            // Clean up resources before exit
            close(uds_stream_sock);
            exit(1);
        }
        
        // Create and configure UDS datagram socket
        if ((uds_dgram_sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
            perror("UDS datagram socket creation failed");
            // Clean up resources before exit
            close(uds_stream_sock);
            unlink(stream_path);
            exit(1);
        }
        
        // Configure UDS datagram socket address
        memset(&uds_dgram_addr, 0, sizeof(uds_dgram_addr));
        uds_dgram_addr.sun_family = AF_UNIX;
        strncpy(uds_dgram_addr.sun_path, datagram_path, sizeof(uds_dgram_addr.sun_path) - 1);
        
        // Remove existing socket file if present
        unlink(datagram_path);
        
        // Bind UDS datagram socket
        if (bind(uds_dgram_sock, (struct sockaddr*)&uds_dgram_addr, sizeof(uds_dgram_addr)) < 0) {
            perror("UDS datagram bind failed");
            // Clean up resources before exit
            close(uds_stream_sock);
            close(uds_dgram_sock);
            unlink(stream_path);
            exit(1);
        }
        
        printf("UDS mode initialized successfully\n");
    }
    
    // Set TCP socket to listen mode with a backlog of 10 connections if in TCP/UDP mode
    if (tcp_sock != -1) {
        if (listen(tcp_sock, 10) < 0) {
            perror("TCP listen failed");
            // Clean up resources before exit
            close(tcp_sock);
            close(udp_sock);
            exit(1);
        }
    }
    
    // Set UDS stream socket to listen mode with a backlog of 10 connections if in UDS mode
    if (uds_stream_sock != -1) {
        if (listen(uds_stream_sock, 10) < 0) {
            perror("UDS stream listen failed");
            // Clean up resources before exit
            close(uds_stream_sock);
            close(uds_dgram_sock);
            unlink(stream_path);
            unlink(datagram_path);
            exit(1);
        }
    }
    
    /* Updated startup message to include all active connections */
    printf("Drinks Bar Server listening:");
    if (TCP_port != -1) printf(" TCP on port %d", TCP_port);
    if (UDP_port != -1) printf(", UDP on port %d", UDP_port);
    if (stream_path != NULL) printf(", UDS stream on %s", stream_path);
    if (datagram_path != NULL) printf(", UDS datagram on %s", datagram_path);
    printf("\n");
    printf("Console commands: GEN SOFT DRINK / GEN VODKA / GEN CHAMPAGNE\n");
    printf("Type exit/quit to exit\n");
    
    print_stock();

    // Initialize file descriptor sets for select()
    fd_set readfds, master_set;
    FD_ZERO(&master_set);
    FD_SET(STDIN_FILENO, &master_set);  // Add stdin to monitoring set for console input
    int maxfd = STDIN_FILENO;  // Start with stdin
   
    // Add all active sockets to monitoring set
    if (tcp_sock != -1) {
        FD_SET(tcp_sock, &master_set);
        maxfd = (tcp_sock > maxfd) ? tcp_sock : maxfd;
    }
    
    if (udp_sock != -1) {
        FD_SET(udp_sock, &master_set);
        maxfd = (udp_sock > maxfd) ? udp_sock : maxfd;
    }
    
    if (uds_stream_sock != -1) {
        FD_SET(uds_stream_sock, &master_set);
        maxfd = (uds_stream_sock > maxfd) ? uds_stream_sock : maxfd;
    }
    
    if (uds_dgram_sock != -1) {
        FD_SET(uds_dgram_sock, &master_set);
        maxfd = (uds_dgram_sock > maxfd) ? uds_dgram_sock : maxfd;
    }

    // Store UDS paths in global variables for signal handler cleanup
    global_stream_path = stream_path;
    global_datagram_path = datagram_path;
    
    // If timeout is set, configure signal handler
    if (timeout > 0) {
        signal(SIGALRM, handle_alarm); 
        printf("Server will automatically shut down after %d seconds of inactivity\n", timeout);
    }
    
    // Main server loop
    while (1) {
        // Make a copy of the master set for select()
        readfds = master_set;

        // Set alarm only if timeout is defined
        if (timeout > 0) {
            alarm(timeout);  // Set alarm to trigger after timeout seconds of inactivity
        }
        
        // Wait for activity on any of the sockets (including stdin)
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1) {
            // Check if the error was caused by the signal interrupt
            if (errno == EINTR) {
                continue;  // If interrupted by signal, just continue the loop
            }
            perror("select");
            exit(1);
        }
        
        // Activity detected, cancel the alarm
        if (timeout > 0) {
            alarm(0);  // Cancel the alarm since we had activity
        }
        
        // Check all file descriptors for activity
        for (int i = 0; i <= maxfd; i++) {
            if (FD_ISSET(i, &readfds)) {
                // ===== TCP CONNECTION HANDLING =====
                if (tcp_sock != -1 && i == tcp_sock) {
                    // New TCP client connection
                    struct sockaddr_in client_addr;
                    socklen_t addrlen = sizeof(client_addr);
                    int new_fd = accept(tcp_sock, (struct sockaddr*)&client_addr, &addrlen);
                    if (new_fd == -1) {
                        perror("TCP accept");
                    } else if (connected_clients >= MAX_CLIENTS) {
                        printf("TCP connection rejected: maximum clients limit reached\n");
                        close(new_fd);
                    } else {
                        // Add new client to the monitoring set
                        FD_SET(new_fd, &master_set);
                        if (new_fd > maxfd) maxfd = new_fd;
                        connected_clients++;
                        printf("New TCP client connected (total: %d)\n", connected_clients);
                    }
                } 
                // ===== UDS STREAM CONNECTION HANDLING =====
                else if (uds_stream_sock != -1 && i == uds_stream_sock) {
                    // New UDS stream client connection
                    struct sockaddr_un client_addr;
                    socklen_t addrlen = sizeof(client_addr);
                    int new_fd = accept(uds_stream_sock, (struct sockaddr*)&client_addr, &addrlen);
                    if (new_fd == -1) {
                        perror("UDS stream accept");
                    } else if (connected_clients >= MAX_CLIENTS) {
                        printf("UDS stream connection rejected: maximum clients limit reached\n");
                        close(new_fd);
                    } else {
                        // Add new client to the monitoring set
                        FD_SET(new_fd, &master_set);
                        if (new_fd > maxfd) maxfd = new_fd;
                        connected_clients++;
                        printf("New UDS stream client connected (total: %d)\n", connected_clients);
                    }
                }
                // ===== UDP DATAGRAM HANDLING =====
                else if (udp_sock != -1 && i == udp_sock) {
                    // UDP datagram received
                    struct sockaddr_in client_addr;
                    socklen_t addrlen = sizeof(client_addr);
                    int n = recvfrom(udp_sock, buffer, sizeof(buffer), 0,
                                    (struct sockaddr*)&client_addr, &addrlen);
                    if (n < 0) {
                        perror("UDP recvfrom");
                    } else {
                        buffer[n] = '\0';  // Null-terminate the received data
                        process_udp_command(buffer, stock_ptr, udp_sock, (struct sockaddr*)&client_addr, addrlen);
                    }
                }
                // ===== UDS DATAGRAM HANDLING =====
                else if (uds_dgram_sock != -1 && i == uds_dgram_sock) {
                    // UDS datagram received
                    struct sockaddr_un client_addr;
                    socklen_t addrlen = sizeof(client_addr);
                    int n = recvfrom(uds_dgram_sock, buffer, sizeof(buffer), 0,
                                    (struct sockaddr*)&client_addr, &addrlen);
                    if (n < 0) {
                        perror("UDS datagram recvfrom");
                    } else {
                        buffer[n] = '\0';  // Null-terminate the received data
                        process_udp_command(buffer, stock_ptr, uds_dgram_sock, (struct sockaddr*)&client_addr, addrlen);
                    }
                } 
                // ===== CONSOLE INPUT HANDLING =====
                else if (i == STDIN_FILENO) {
                    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                        size_t len = strlen(buffer);
                        if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = '\0';  // Remove newline
                        
                        // Handle exit commands
                        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) {
                            printf("Exiting...\n");
                            if (lock_fd != -1) close(lock_fd);
                            if (tcp_sock != -1) close(tcp_sock);
                            if (udp_sock != -1) close(udp_sock);
                            if (uds_stream_sock != -1) { 
                                close(uds_stream_sock); 
                                if (stream_path != NULL) unlink(stream_path); 
                            }
                            if (uds_dgram_sock != -1) { 
                                close(uds_dgram_sock); 
                                if (datagram_path != NULL) unlink(datagram_path); 
                            }
                            exit(0);
                        }
                        process_console_command(buffer, stock_ptr);
                    }
                }
                // ===== EXISTING CLIENT CONNECTION HANDLING =====
                else if (i != STDIN_FILENO) {
                    // Data received from an existing TCP or UDS stream client
                    ssize_t n = recv(i, buffer, sizeof(buffer) - 1, 0);
                    if (n <= 0) {
                        // Client disconnected or error occurred
                        close(i);
                        FD_CLR(i, &master_set);
                        connected_clients--;
                        printf("Client disconnected (remaining: %d)\n", connected_clients);
                    } else {
                        buffer[n] = '\0';  // Null-terminate the received data
                        process_tcp_command(buffer, stock_ptr, i);
                    }
                }
            }
        }
    }
}