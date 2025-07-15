# Operating Systems Assignment 2 - Molecular Warehouse System

## 📋 Authors
- **Student Name**: Ido Cohen  
- **Student Name**: Eitan Halimi

---

## 🎯 Project Overview

This project implements a sophisticated **molecular warehouse management system** that demonstrates advanced concepts in systems programming, network communication, and inter-process coordination. The system evolves through 6 progressive stages, each introducing new capabilities and technologies.

The warehouse manages three types of atoms (**Carbon**, **Hydrogen**, **Oxygen**) and can produce complex molecules (**H₂O**, **CO₂**, **C₂H₆O**, **C₆H₁₂O₆**) and beverages (**Soft Drinks**, **Vodka**, **Champagne**) based on chemical formulas.

### 🔬 Key Capabilities
- **Multi-Protocol Communication**: TCP, UDP, Unix Domain Sockets
- **Concurrent Processing**: Handle multiple clients simultaneously  
- **Persistent Storage**: Memory-mapped files with process synchronization
- **Real-time Inventory**: Live atom counting with overflow protection (up to 10¹⁸ atoms)
- **Drink Production**: Calculate beverage quantities based on molecular recipes

---

## 📁 Project Structure

```
os_2/
├── q1/                    # Basic TCP Client-Server Architecture
│   ├── atom_warehouse.c   # TCP server for atom management
│   ├── atom_supplier.c    # Interactive TCP client
│   └── Makefile
├── q2/                    # Multi-Protocol Server (TCP + UDP)
│   ├── molecule_supplier.c # Dual-protocol server
│   ├── molecule_requester.c # Universal client
│   └── Makefile
├── q3/                    # Administrative Console Interface
│   ├── drinks_bar.c       # Enhanced server with drink calculations
│   ├── atom_supplier.c    # TCP client for atom addition (from Q1)
│   ├── molecule_requester.c # UDP client for molecule requests (from Q2)
│   └── Makefile
├── q4/                    # Command-Line Interface & Timeout Support
│   ├── drinks_bar.c       # Feature-complete server with CLI options
│   ├── atom_supplier.c    # Enhanced client with argument parsing
│   ├── molecule_requester.c # Enhanced client with argument parsing
│   └── Makefile
├── q5/                    # Unix Domain Sockets Implementation
│   ├── drinks_bar.c       # Multi-transport server
│   ├── atom_supplier.c    # Multi-transport client
│   ├── molecule_requester.c # Multi-transport client
│   └── Makefile
├── q6/                    # Persistent Storage & Process Synchronization
│   ├── drinks_bar.c       # Production-ready server with mmap
│   ├── atom_supplier.c    # Final client implementation
│   ├── molecule_requester.c # Final client implementation
│   ├── coverage_report_q6.txt # Code coverage analysis
│   └── Makefile
├── Makefile               # Recursive build system
├── מטלה.txt              # Assignment specifications (Hebrew)
└── README.md              # This documentation
```

---

## 🚀 Features by Stage

### **Q1: Foundation - Basic TCP Warehouse** (15 points)
**Executable**: `atom_warehouse`, `atom_supplier`

🔧 **Core Functionality**:
- **TCP Server**: Listens for client connections and manages atom inventory
- **Interactive Client**: Menu-driven interface for atom additions
- **Inventory Management**: Real-time tracking of Carbon, Hydrogen, Oxygen atoms
- **Overflow Protection**: Validates operations against 10¹⁸ atom limit

**Key Technologies**:
- `socket()`, `bind()`, `listen()`, `accept()`
- `select()` for I/O multiplexing
- Client connection management

---

### **Q2: Evolution - Molecule Production** (15 points)
**Executable**: `molecule_supplier`, `molecule_requester`

🔧 **Enhanced Functionality**:
- **Dual-Protocol Server**: Simultaneous TCP (atoms) + UDP (molecules) support
- **Molecule Synthesis**: Automated production of complex molecules
- **Chemical Formulas**: 
  - **H₂O** (Water): 2H + 1O
  - **CO₂** (Carbon Dioxide): 1C + 2O  
  - **C₂H₆O** (Alcohol): 2C + 6H + 1O
  - **C₆H₁₂O₆** (Glucose): 6C + 12H + 6O

**Key Technologies**:
- UDP datagram processing
- Multi-protocol socket management
- Molecular arithmetic and validation

---

### **Q3: Administration - Beverage Production Console** (15 points)
**Executable**: `drinks_bar`, `molecule_requester`

🔧 **Administrative Features**:
- **Console Interface**: Real-time administrative commands via stdin
- **Beverage Calculations**: Production capacity analysis
- **Recipe Management**:
  - **Soft Drink**: H₂O + CO₂ + C₆H₁₂O₆
  - **Vodka**: H₂O + C₂H₆O + C₆H₁₂O₆  
  - **Champagne**: H₂O + CO₂ + C₂H₆O
- **Concurrent Operations**: Simultaneous client + admin operations

**Key Technologies**:
- Multi-source input handling (TCP + UDP + stdin)
- Complex recipe calculations
- Administrative command processing

---

### **Q4: Professionalization - CLI & Timeout Support**
**Executable**: `drinks_bar`, `atom_supplier`, `molecule_requester`

🔧 **Professional Features**:
- **Advanced CLI**: Full `getopt_long()` argument parsing
- **Initial Configuration**: Pre-populate inventory from command line
- **Timeout Management**: `SIGALRM`-based automatic shutdown
- **Flexible Addressing**: Support for both IP addresses and hostnames

**Command Line Options**:
```bash
# Server Options
-T, --tcp-port <port>      # TCP listening port (required)
-U, --udp-port <port>      # UDP listening port (required)  
-o, --oxygen <count>       # Initial oxygen atoms
-c, --carbon <count>       # Initial carbon atoms
-h, --hydrogen <count>     # Initial hydrogen atoms
-t, --timeout <seconds>    # Inactivity timeout

# Client Options  
-h <hostname/IP>           # Server hostname or IP
-p <port>                  # Server port
```

**Key Technologies**:
- `getopt_long()` for robust argument parsing
- `alarm()` and signal handling for timeouts
- `getaddrinfo()` for hostname resolution

---

### **Q5: Modernization - Unix Domain Sockets**
**Executable**: `drinks_bar`, `atom_supplier`, `molecule_requester`

🔧 **Transport Layer Flexibility**:
- **Multi-Transport Support**: Network sockets + Unix Domain Sockets
- **UDS Stream**: TCP-equivalent for local inter-process communication
- **UDS Datagram**: UDP-equivalent for local message passing
- **Automatic Cleanup**: Socket file management and cleanup

**UDS Command Line Options**:
```bash
# Server UDS Options
-s, --stream-path <path>     # UDS stream socket path  
-d, --datagram-path <path>   # UDS datagram socket path

# Client UDS Options
-f <UDS-socket-path>         # Connect via Unix Domain Socket
```

**Key Technologies**:
- `AF_UNIX` socket family
- `SOCK_STREAM` and `SOCK_DGRAM` for UDS
- Filesystem-based socket addressing
- Socket file lifecycle management

---

### **Q6: Production-Ready - Persistent Storage & Concurrency**
**Executable**: `drinks_bar`, `atom_supplier`, `molecule_requester`

🔧 **Enterprise-Grade Features**:
- **Memory-Mapped Persistence**: Zero-copy file I/O using `mmap()`
- **Process Synchronization**: File locking for concurrent server instances  
- **State Recovery**: Automatic inventory restoration across restarts
- **Atomic Operations**: Race-condition-free inventory updates

**Persistence Command Line**:
```bash
-f, --save-file <filepath>   # Persistent storage file
```

**Advanced Implementation Details**:
- **File Locking Strategy**:
  - `LOCK_EX` (Exclusive) for write operations (add/subtract atoms)
  - `LOCK_SH` (Shared) for read operations (display inventory/calculate drinks)
  - `LOCK_UN` for lock release
- **Memory Mapping**: `mmap()` with `MAP_SHARED` for inter-process visibility
- **State Management**: 
  - **Existing file**: Load current inventory, ignore CLI atom counts
  - **New file**: Initialize with CLI values, create persistent storage
  - **No file**: Traditional in-memory operation (backward compatibility)

**Key Technologies**:
- `mmap()` and `munmap()` for memory-mapped I/O
- `flock()` for file-based synchronization
- `ftruncate()` for file size management
- Multi-process coordination patterns

---

## 🔧 Compilation & Build System

### **Build All Stages**
```bash
make all
```

### **Build Individual Stages**
```bash
cd q1 && make    # Basic TCP system
cd q2 && make    # Multi-protocol system  
cd q3 && make    # Administrative console
cd q4 && make    # CLI & timeout support
cd q5 && make    # Unix Domain Sockets
cd q6 && make    # Persistent storage
```

### **Clean All Builds**
```bash
make clean
```

---

## 📖 Usage Examples

### **Q1: Basic TCP Warehouse**
**Executables**: `atom_warehouse`, `atom_supplier`

```bash
# Terminal 1 - Start atom warehouse server
cd q1
./atom_warehouse <TCP_PORT>
# Example: ./atom_warehouse 12345

# Terminal 2 - Connect atom supplier client  
./atom_supplier <HOSTNAME/IP> <TCP_PORT>
# Example: ./atom_supplier 127.0.0.1 12345
# Example: ./atom_supplier localhost 12345
```

### **Q2: Molecule Production System**
**Executables**: `molecule_supplier`, `molecule_requester`

```bash
# Terminal 1 - Start molecule supplier server
cd q2
./molecule_supplier <TCP_PORT> <UDP_PORT>
# Example: ./molecule_supplier 12345 12346

# Terminal 2 - Start molecule requester client
./molecule_requester <HOSTNAME/IP> <PORT>
# Example: ./molecule_requester 127.0.0.1 12345
# Example: ./molecule_requester localhost 12345
```

### **Q3: Administrative Beverage Console**
**Executables**: `drinks_bar`, `atom_supplier`, `molecule_requester`

```bash
# Terminal 1 - Start drinks bar server with admin console
cd q3
./drinks_bar <TCP_PORT> <UDP_PORT>
# Example: ./drinks_bar 12345 12346
# Server console supports GEN commands for drink calculations

# Terminal 2 - Connect atom supplier client (TCP - for adding atoms)
./atom_supplier <HOSTNAME/IP> <TCP_PORT>
# Example: ./atom_supplier 127.0.0.1 12345

# Terminal 3 - Connect molecule requester client (UDP - for requesting molecules)
./molecule_requester <HOSTNAME/IP> <UDP_PORT>
# Example: ./molecule_requester 127.0.0.1 12345
# Note: Client connects to TCP port, UDP communication is handled automatically
```

### **Q4: Professional CLI Server**
**Executables**: `drinks_bar`, `atom_supplier`, `molecule_requester`

**Server Options**:
```bash
cd q4
./drinks_bar -T <tcp-port> -U <udp-port> [OPTIONS]

Required:
  -T, --tcp-port <port>      TCP listening port
  -U, --udp-port <port>      UDP listening port

Optional:
  -o, --oxygen <count>       Initial oxygen atoms
  -c, --carbon <count>       Initial carbon atoms  
  -h, --hydrogen <count>     Initial hydrogen atoms
  -t, --timeout <seconds>    Inactivity timeout

# Examples:
./drinks_bar -T 12345 -U 12346
./drinks_bar -T 12345 -U 12346 -c 1000 -o 2000 -h 3000 -t 60
```

**Client Options**:
```bash
# Atom supplier client
./atom_supplier -h <hostname/IP> -p <port>
# Example: ./atom_supplier -h localhost -p 12345

# Molecule requester client  
./molecule_requester -h <hostname/IP> -p <port>
# Example: ./molecule_requester -h 127.0.0.1 -p 12345
```

### **Q5: Unix Domain Sockets**
**Executables**: `drinks_bar`, `atom_supplier`, `molecule_requester`

**Server Transport Options**:
```bash
cd q5
# Option 1: Network Sockets (TCP/UDP)
./drinks_bar -T <tcp-port> -U <udp-port> [ATOM_OPTIONS] [TIMEOUT]

# Option 2: Unix Domain Sockets
./drinks_bar -s <stream-path> -d <datagram-path> [ATOM_OPTIONS] [TIMEOUT]

# Examples:
./drinks_bar -T 12345 -U 12346 -c 1000 -o 2000 -h 3000
./drinks_bar -s /tmp/warehouse_stream.sock -d /tmp/warehouse_dgram.sock -c 1000
```

**Client Transport Options**:
```bash
# Option 1: Network Connection
./atom_supplier -h <hostname/IP> -p <port>
./molecule_requester -h <hostname/IP> -p <port>

# Option 2: Unix Domain Socket Connection
./atom_supplier -f <UDS_socket_path>
./molecule_requester -f <UDS_socket_path>

# Examples:
./atom_supplier -h 127.0.0.1 -p 12345
./atom_supplier -f /tmp/warehouse_stream.sock
./molecule_requester -f /tmp/warehouse_stream.sock
```

### **Q6: Persistent Multi-Process System**
**Executables**: `drinks_bar`, `atom_supplier`, `molecule_requester`

**Server Persistence Options**:
```bash
cd q6
# Option 1: Network + Persistence
./drinks_bar -T <tcp-port> -U <udp-port> [OPTIONS] [-f <save-file>]

# Option 2: UDS + Persistence  
./drinks_bar -s <stream-path> -d <datagram-path> [OPTIONS] [-f <save-file>]

All Server Options:
  -T, --tcp-port <port>        TCP listening port
  -U, --udp-port <port>        UDP listening port  
  -s, --stream-path <path>     UDS stream socket path
  -d, --datagram-path <path>   UDS datagram socket path
  -o, --oxygen <count>         Initial oxygen atoms
  -c, --carbon <count>         Initial carbon atoms
  -h, --hydrogen <count>       Initial hydrogen atoms
  -t, --timeout <seconds>      Inactivity timeout
  -f, --save-file <filepath>   Persistent storage file

# Examples:
./drinks_bar -T 12345 -U 12346 -f warehouse.dat -c 5000 -o 3000 -h 7000
./drinks_bar -s /tmp/stream.sock -d /tmp/dgram.sock -f warehouse.dat -c 1000
```

**Client Connection Options**:
```bash
# Option 1: Network Connection
./atom_supplier -h <hostname/IP> -p <port>
./molecule_requester -h <hostname/IP> -p <port>

# Option 2: Unix Domain Socket Connection
./atom_supplier -f <UDS_socket_path>
./molecule_requester -f <UDS_socket_path>

# Examples:
./atom_supplier -h 127.0.0.1 -p 12345
./molecule_requester -h localhost -p 12345
./atom_supplier -f /tmp/stream.sock
./molecule_requester -f /tmp/stream.sock
```
---

## 🎮 Supported Commands

### **Client Commands (TCP/Stream Connection)**
| Command | Description | Example |
|---------|-------------|---------|
| `ADD CARBON <amount>` | Add carbon atoms to inventory | `ADD CARBON 1000` |
| `ADD HYDROGEN <amount>` | Add hydrogen atoms to inventory | `ADD HYDROGEN 2000` |  
| `ADD OXYGEN <amount>` | Add oxygen atoms to inventory | `ADD OXYGEN 1500` |

### **Client Commands (UDP/Datagram Connection)**
| Command | Description | Formula | Example |
|---------|-------------|---------|---------|
| `DELIVER WATER <qty>` | Request water molecules | H₂O (2H + 1O) | `DELIVER WATER 100` |
| `DELIVER CARBON DIOXIDE <qty>` | Request CO₂ molecules | CO₂ (1C + 2O) | `DELIVER CARBON DIOXIDE 50` |
| `DELIVER ALCOHOL <qty>` | Request alcohol molecules | C₂H₆O (2C + 6H + 1O) | `DELIVER ALCOHOL 25` |
| `DELIVER GLUCOSE <qty>` | Request glucose molecules | C₆H₁₂O₆ (6C + 12H + 6O) | `DELIVER GLUCOSE 10` |

### **Administrative Commands (Server Console - Q3+)**
| Command | Description | Recipe |
|---------|-------------|---------|
| `GEN SOFT DRINK` | Calculate soft drink capacity | H₂O + CO₂ + C₆H₁₂O₆ |
| `GEN VODKA` | Calculate vodka capacity | H₂O + C₂H₆O + C₆H₁₂O₆ |
| `GEN CHAMPAGNE` | Calculate champagne capacity | H₂O + CO₂ + C₂H₆O |

---

## 🔬 Technical Implementation Deep Dive

### **Network Programming**
- **Multi-Protocol Architecture**: Concurrent TCP and UDP socket handling
- **I/O Multiplexing**: `select()` for efficient multi-client management
- **Address Resolution**: `getaddrinfo()` for robust hostname/IP handling
- **Error Recovery**: Comprehensive network error handling and client cleanup

### **Inter-Process Communication**  
- **Unix Domain Sockets**: High-performance local communication
- **Socket Lifecycle**: Automatic socket file creation and cleanup
- **Transport Abstraction**: Unified client interface across network and local sockets

### **Memory Management & Persistence**
- **Memory-Mapped I/O**: `mmap()` for zero-copy persistent storage
- **File Synchronization**: `msync()` for immediate data persistence  
- **Resource Management**: Proper cleanup of memory mappings and file descriptors

### **Concurrency & Synchronization**
- **File Locking**: `flock()` with advisory locking for process coordination
- **Atomic Operations**: Transaction-like inventory updates
- **Lock Granularity**: Optimized shared/exclusive locking strategy

### **Signal Handling**
- **Timeout Management**: `SIGALRM` for automatic server shutdown
- **Graceful Cleanup**: Signal handlers for resource deallocation
- **Signal Safety**: Async-signal-safe cleanup operations

---

## 🧪 Testing & Quality Assurance

### **Code Coverage Analysis**
The project includes comprehensive code coverage reporting:

```bash
cd q6
# Generate coverage data (requires --coverage flag in Makefile)
gcov *.c

# Coverage statistics automatically generated in coverage_report_q6.txt
```

**Current Coverage Metrics** (Q6):
- **atom_supplier.c**: 84.40% line coverage
- **drinks_bar.c**: 80.43% line coverage  
- **molecule_requester.c**: 84.17% line coverage
- **Overall**: 81.79% combined coverage

### **Error Handling Coverage**
The system includes comprehensive error handling for:
- ✅ Network connectivity failures
- ✅ Invalid command formats
- ✅ Atom count overflow (>10¹⁸ limit)
- ✅ Insufficient atoms for molecule production
- ✅ File I/O errors and permission issues
- ✅ Memory allocation failures
- ✅ Concurrent access conflicts

---

## 🎯 Educational Objectives Achieved

### **Core Systems Programming Concepts**
1. **Socket Programming**: TCP/UDP network communication
2. **I/O Multiplexing**: `select()` for concurrent client handling
3. **Process Synchronization**: File locking and shared resources
4. **Memory Management**: Dynamic allocation and memory-mapped files
5. **Signal Handling**: Timeout mechanisms and graceful shutdown

### **Advanced IPC Mechanisms** 
1. **Unix Domain Sockets**: High-performance local communication
2. **Memory-Mapped Files**: Zero-copy persistent storage
3. **File Locking**: Advisory locking for multi-process coordination
4. **Atomic Operations**: Race-condition-free shared state management

### **Professional Development Practices**
1. **Modular Design**: Progressive feature development across stages
2. **Error Handling**: Comprehensive validation and recovery mechanisms  
3. **Documentation**: Extensive inline comments and external documentation
4. **Testing**: Code coverage analysis and systematic validation
5. **Build Systems**: Recursive Makefiles and dependency management

---

## 🔍 System Requirements

### **Compilation Environment**
- **Compiler**: GCC with C99 standard support
- **Required Standards**: `_POSIX_C_SOURCE=200112L`
- **Build Tools**: Make, standard POSIX utilities
---


## 🎓 Assignment Information

**Course**: Operating Systems - Computer Science  
**Institution**: Ariel University  
**Submission Requirements**: Complete source code + coverage reports + recursive Makefile

---

**🔥 This implementation demonstrates mastery of advanced systems programming concepts including network programming, inter-process communication, memory management, and concurrent programming in a production-quality molecular warehouse management system.**
