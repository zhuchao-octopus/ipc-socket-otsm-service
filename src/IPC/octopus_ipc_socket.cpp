/*
 * File: octopus_ipc_socket.cpp
 * Organization: Octopus
 * Description:
 *  This file contains the implementation of the `Socket` class, which handles the creation, binding,
 *  listening, and communication over a Unix Domain Socket (UDS). The class supports both server-side
 *  and client-side socket operations such as opening a socket, sending queries and responses, and
 *  handling client connections.
 *
 *  - Server-side operations include socket creation, binding, listening, accepting client connections,
 *    reading client queries, and sending responses.
 *  - Client-side operations include connecting to the server, sending queries, and receiving responses.
 *
 * Author		: [ak47]
 * Organization	: [octopus]
 * Date Time	: [2025/0313/21:00]
 */

#include "octopus_ipc_socket.hpp"
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cstring>
#include <string>
#include <sys/stat.h>

// Signal handler for server shutdown on interruption signals
static void signal_handler(int signum)
{
    std::cout << "Octopus IPC Socket Received Signal: " << signum << std::endl;
    std::exit(-1);
}

// Constructor for the Socket class
Socket::Socket()
{
    // Initialize socket properties
    socket_path = "/tmp/octopus/ipc_socket";
    domain = AF_UNIX;   // Domain type: Unix domain socket
    type = SOCK_STREAM; // Type: Stream socket (TCP-like behavior)
    protocol = 0;       // Protocol: Default

    // Setup signal handling for interrupts
    signal(SIGINT, signal_handler);
    signal(SIGKILL, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize socket address structure for binding
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;                                      // Set address family to Unix domain
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1); // Set socket path

    // Server configuration
    max_waiting_requests = 5; // Max number of clients waiting to be served
}

// Open a socket for communication
int Socket::open_socket()
{
    socket_fd = socket(domain, type, protocol); // Create the socket

    // Error handling if socket creation fails
    if (socket_fd == -1)
    {
        std::cerr << "socket could not be created." << std::endl;
        close_socket(); // Ensure socket is closed if creation fails
    }

    return socket_fd;
}

// Close the socket
int Socket::close_socket()
{
    close(socket_fd); // Close the socket file descriptor
    if (socket_fd >= 0)
        std::cout << "close socket [" << socket_fd << "]" << std::endl;
    return socket_fd;
}

// Bind the server to the socket address
void Socket::bind_server_to_socket()
{
    // Binding socket to the address
    if (bind(socket_fd, (sockaddr *)&addr, sizeof(addr)) == -1)
    {
        std::cerr << "Server Socket could not be bound to socket." << std::endl;
        close_socket();
    }

    std::cout << "Server socket opened with [" << socket_fd << "] " << addr.sun_family << " domain" << std::endl;
    std::cout << "Server socket opened with [" << socket_fd << "] " << addr.sun_path << " path" << std::endl;

    // Set permissions for the socket file to allow clients to access
    int permission = chmod(socket_path, S_IRWXU | S_IRWXG | S_IRWXO);
    if (permission == -1)
    {
        std::cerr << "Server Socket file path could not give chmod permission to client." << std::endl;
        close_socket();
    }
}

// Start the server to listen for incoming connections
void Socket::start_listening()
{
    std::cout << "Server started to listening." << std::endl;

    int listen_result = listen(socket_fd, max_waiting_requests); // Start listening with a queue size

    // Handle errors in listening
    if (listen_result == -1)
    {
        std::cerr << "Server Listen failed." << std::endl;
        close_socket();
    }
}

// Accept an incoming client connection
int Socket::wait_and_accept()
{
    // Wait for a client to connect and accept the connection
    int client_fd = accept(socket_fd, NULL, NULL);

    // Handle errors in accepting the connection
    if (client_fd == -1)
    {
        std::cerr << "Server: Client connection could not be accepted." << std::endl;
    }

    std::cout << "Server accepted client connection [" << client_fd << "]" << std::endl;
    return client_fd;
}

// Read the query from the client
std::vector<int> Socket::get_query(int client_fd)
{
    char buffer[IPC_SOCKET_QUERY_BUFFER_SIZE];                            // Buffer for storing the query
    query_bytesRead = read(client_fd, buffer, sizeof(buffer)); // Read the client query

    // Handle read errors
    if (query_bytesRead <= 0)
    {
        return {}; // Return an empty vector if reading failed
    }

    // Convert the buffer into a vector of integers
    std::vector<int> query_vec(sizeof(buffer));
    for (int i = 0; i < sizeof(buffer); i++)
    {
        query_vec[i] = buffer[i];
    }

    return query_vec;
}

// Send a response to the client
int Socket::send_response(int client_fd, std::vector<int> &resp_vector)
{
    char resp_buffer[resp_vector.size()]; // Buffer for the response
    for (int i = 0; i < resp_vector.size(); i++)
    {
        resp_buffer[i] = resp_vector[i];
    }
    auto write_result = write(client_fd, resp_buffer, sizeof(resp_buffer)); // Send the response

    // Handle errors in sending the response
    if (write_result == -1)
    {
        std::cerr << "Server Could not write response to client." << std::endl;
        close_socket();
    }

    return 0;
}

// Send a response to the client
int Socket::send_buff(int client_fd, char *resp_buffer, int length)
{
    // char resp_buffer[resp_vector.size()]; // Buffer for the response
    // for (int i = 0; i < resp_vector.size(); i++)
    //{
    //     resp_buffer[i] = resp_vector[i];
    // }
    auto write_result = write(client_fd, resp_buffer, length); // Send the response

    // Handle errors in sending the response
    if (write_result == -1)
    {
        std::cerr << "Server Could not write response to client." << std::endl;
        close_socket();
        return -1;
    }

    return 0;
}

// Connect the client to the server
int Socket::connect_to_socket()
{
    // Attempt to connect to the server socket
    if (connect(socket_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        std::cerr << "Client: could not connect to server socket [" << socket_fd << "]" << std::endl;
        return -1; // Return -1 on failure
    }

    return socket_fd;
}

// Send a query from the client to the server
void Socket::send_query(std::vector<int> &query_vector)
{
    char query_buffer[query_vector.size()]; // Buffer for the query
    for (int i = 0; i < query_vector.size(); i++)
    {
        query_buffer[i] = query_vector[i];
    }
    auto write_result = write(socket_fd, query_buffer, sizeof(query_buffer)); // Send the query

    // Handle errors in sending the query
    if (write_result == -1)
    {
        std::cerr << "Client: Could not write query to socket." << std::endl;
        close_socket();
    }
}

// Receive the response from the server
std::pair<std::vector<int>, int> Socket::get_response()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Introduce a short delay

    char result_buffer[IPC_SOCKET_RESPONSE_BUFFER_SIZE];                                      // Buffer for the response
    int resp_bytesRead = read(socket_fd, result_buffer, sizeof(result_buffer)); // Read the response
    std::cout << "get_response " << resp_bytesRead <<"/"<< sizeof(result_buffer)<< std::endl; // 
    // Handle errors in receiving the response
    if (resp_bytesRead == -1)
    {
        std::cerr << "Client: Could not read response from server." << std::endl;
        close_socket();
        return {{}, 0}; // Return an empty vector with size 0
    }

    // Convert the response buffer into a vector of integers (only the valid received bytes)
    std::vector<int> result_vec(resp_bytesRead);
    for (int i = 0; i < resp_bytesRead; i++)
    {
        result_vec[i] = static_cast<unsigned char>(result_buffer[i]); // Ensure proper unsigned conversion
    }

    return {result_vec, resp_bytesRead}; // Return the received data and size
}

void Socket::printf_vector_bytes(const std::vector<int> &vec, int length)
{
    //std::cout << "printf_vector_bytes " << length << std::endl; // 
    // 确保 length 不超过 vector 的大小
    int print_length = std::min(length, static_cast<int>(vec.size()));

    for (int i = 0; i < print_length; ++i)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                 <<"0x" << (vec[i] & 0xFF) << " "; // 打印每个字节
    }
    std::cout << std::dec << std::endl; // 恢复十进制格式
}
// 打印 vector 中的前 count 个字节
void Socket::printf_buffer_bytes(const std::vector<int> &vec, int length)
{
    // 确保打印的字节数不超过 vector 的大小
    int print_count = std::min(length, static_cast<int>(vec.size()));

    for (int i = 0; i < print_count; ++i)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (vec[i] & 0xFF) << " "; // 取低 8 位当作 1 字节
    }
    std::cout << std::dec << std::endl; // 恢复十进制格式
}

// 打印指定字节数的 buffer
void Socket::printf_buffer_bytes(const void *buffer, int length)
{
    const uint8_t *byte_ptr = static_cast<const uint8_t *>(buffer);

    // 确保打印的字节数不超过 buffer 的大小
    int print_count = length;

    for (int i = 0; i < print_count; ++i)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(byte_ptr[i]) << " ";
    }
    std::cout << std::dec << std::endl; // 恢复十进制格式
}
