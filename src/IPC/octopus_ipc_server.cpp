/*
 * File: octopus_ipc_server.cpp
 * Description: 
 *  This file implements a simple multi-threaded IPC server that listens for client connections
 *  over a Unix domain socket. It supports basic arithmetic operations such as addition, subtraction,
 *  multiplication, and division. Each client connection is handled in a separate thread to allow
 *  simultaneous connections. The server also ensures the socket directory exists and removes old
 *  socket files before starting up.
 * 
 * Includes:
 *  - A socket server class that manages the opening, binding, listening, and communication with clients.
 *  - Mutexes to ensure thread safety when modifying shared resources like active client connections.
 *  - Signal handling for graceful cleanup upon interrupt.
 * 
 * Compilation:
 *  This file requires the following libraries:
 *   - <mutex>        : For thread synchronization using mutexes.
 *   - <atomic>       : For atomic operations (unused in this example, can be added later).
 *   - <unordered_set>: For storing active client connections in a set.
 *   - <sys/stat.h>   : For checking and creating directories.
 *   - <unistd.h>     : For removing old socket files.
 *   - <iostream>     : For logging messages to the console.
 *   - <vector>       : For storing query and response data.
 * 
 * Author		: [ak47]
 * Organization	: [octopus]
 * Date Time	: [2025/0313/21:00]
 */

#include <mutex>
#include <atomic>
#include <unordered_set>
#include "octopus_ipc_socket.hpp"
#include "../HAL/octopus_serialport.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to handle client communication
void handle_client(int client_fd);

// Path for the IPC socket file
const char* socket_path = "/tmp/octopus/ipc_socket";

// Server object to handle socket operations
Socket server;

// Mutex for server operations to ensure thread-safety
std::mutex server_mutex;

// Unordered set to track active clients
std::unordered_set<int> active_clients;

// Mutex for client operations to ensure thread-safety
std::mutex clients_mutex;

SerialPort serial("/dev/ttyS0", B115200);
////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to ensure the directory for the socket file exists
void ensure_directory_exists(const char* path) {
    std::string dir = path;
    size_t pos = dir.find_last_of('/');

    if (pos != std::string::npos) {
        std::string parent_dir = dir.substr(0, pos);
        struct stat st;

        // If the parent directory does not exist, create it
        if (stat(parent_dir.c_str(), &st) == -1) {
            if (mkdir(parent_dir.c_str(), 0777) == -1) {
                std::cerr << "Failed to create directory: " << strerror(errno) << std::endl;
            }
        }
    }
}

// Function to remove the old socket file if it exists
void remove_old_socket() {
    unlink(socket_path);
}

// Signal handler for clean-up on interrupt (e.g., Ctrl+C)
void signal_handler(int signum) {
    std::cout << "Server Interrupt signal received. Cleaning up...\n";
    server.close_socket();
    exit(signum);
}

// 回调函数，处理接收到的数据
void onDataReceived(const std::string& data) {
    std::cout << "Callback received data: " << data << std::endl;
}

void initSerialPort()
{
    // 打开串口
    if (!serial.openPort()) {
        std::cerr << "Failed to open serial port!" << std::endl;
    }
    // 设置回调函数，处理接收到的数据
    serial.setCallback(onDataReceived);
}

int main() {
    std::cout << "Server process started." << std::endl;

    // Set up signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, signal_handler);

    // Ensure the directory for the socket exists
    ensure_directory_exists(socket_path);

    // Remove old socket if it exists
    remove_old_socket();

    // Open and bind the server socket
    server.open_socket();
    server.bind_server_to_socket();

    // Start listening for incoming client connections
    server.start_listening();

    // Set buffer sizes for query and response
    server.query_buffer_size = 20;
    server.respo_buffer_size = 20;

    std::cout << "Server waiting for client connections..." << std::endl;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    initSerialPort();
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main loop to accept and handle client connections
    while (true) {
        int client_fd = server.wait_and_accept();

        // If accepting a client fails, continue to the next iteration
        if (client_fd < 0) {
            std::cerr << "Server Failed to accept client connection" << std::endl;
            continue;
        }

        // Lock the clients mutex to safely modify the active clients set
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            active_clients.insert(client_fd);
        }

        // Create a thread to handle the client communication
        std::thread client_thread(handle_client, client_fd);

        // Detach the thread so it runs independently
        client_thread.detach();
    }

    // Close the server socket before exiting
    server.close_socket();
    return 0;
}

// Function to handle communication with a specific client
void handle_client(int client_fd) {
    std::cout << "Server handling client [" << client_fd << "] connected." << std::endl;

    // Buffers to hold query and response data
    std::vector<int> query_vector(3);
    std::vector<int> resp_vector(1);

    // Loop to continuously process client queries
    while (true) {
        // Lock the server mutex to safely fetch the query data
        {
            std::lock_guard<std::mutex> lock(server_mutex);
            query_vector = server.get_query(client_fd);
        }

        // If the query is empty, break the loop and end the communication
        if (query_vector.empty()) {
            std::cout << "Server handling client [" << client_fd << "] empty,existing." << std::endl;
            break;
        }

        int calc_result = 0;

        // Perform the requested operation based on the query
        switch (query_vector[0]) {
        case 1:
            calc_result = query_vector[1] + query_vector[2];
            break;
        case 2:
            calc_result = query_vector[1] - query_vector[2];
            break;
        case 3:
            calc_result = query_vector[1] * query_vector[2];
            break;
        case 4:
            // Check for division by zero
            if (query_vector[2] == 0) {
                std::cerr << "Server handling client [" << client_fd << "]: Error - Division by zero!" << std::endl;
                calc_result = 0;
            } else {
                calc_result = query_vector[1] / query_vector[2];
            }
            break;
        default:
            std::cerr << "Server handling client [" << client_fd << "]: Invalid operation." << std::endl;
            continue;
        }

        // Set the calculated result in the response vector
        resp_vector[0] = calc_result;

        // Lock the server mutex to safely send the response to the client
        {
            std::lock_guard<std::mutex> lock(server_mutex);
            server.send_response(client_fd, resp_vector);
        }

        std::cout << "Server handling client [" << client_fd << "] done." << std::endl;
    }

    // Close the client connection
    close(client_fd);

    // Lock the clients mutex to safely remove the client from the active clients list
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        active_clients.erase(client_fd);
    }

    std::cout << "Server handling client [" << client_fd << "] closed." << std::endl;
}
