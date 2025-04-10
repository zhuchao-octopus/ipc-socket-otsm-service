/*
 * File: octopus_ipc_socket.hpp
 * Organization: Octopus
 * Description:
 *  This header file defines the `Socket` class, which encapsulates the functionalities required to
 *  manage communication through a Unix Domain Socket (UDS). It includes methods for setting up a
 *  server to handle incoming client requests, sending responses, and performing arithmetic operations.
 *  The class also supports client-side socket communication by providing methods for connecting to the
 *  server, sending queries, and receiving responses.
 *
 * Includes:
 *  - Socket setup and connection management through Unix domain sockets.
 *  - Query and response handling for arithmetic operations.
 *  - Server-side socket functions including binding, listening, and accepting client connections.
 *  - Client-side socket functions including query sending and response receiving.
 *
 * Author		: [ak47]
 * Organization	: [octopus]
 * Date Time	: [2025/0313/21:00]
 */
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <mutex>
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
#include <iomanip>
#include <sys/epoll.h>
#include <unordered_map>
#include "octopus_ipc_ptl.hpp"
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define IPC_SOCKET_RESPONSE_BUFFER_SIZE 255
#define IPC_SOCKET_QUERY_BUFFER_SIZE 255
// Structure to store active client information
struct ClientInfo
{
    int fd;         // Client file descriptor
    bool flag;      // Status flag for additional state tracking
    std::string ip; // Client IP address

    // Constructor to initialize client information
    ClientInfo(int fd, std::string ip, bool flag)
        : fd(fd), ip(std::move(ip)), flag(flag) {}

    // Overload == operator to allow comparison in unordered_set
    bool operator==(const ClientInfo &other) const
    {
        return fd == other.fd; // Compare based on fd (assuming it's unique)
    }
};
enum class QueryStatus
{
    Success,
    Timeout,
    Disconnected,
    Error
};

struct QueryResult
{
    QueryStatus status;
    std::vector<uint8_t> data;
};
// Custom hash function specialization for ClientInfo
namespace std
{
    template <>
    struct hash<ClientInfo>
    {
        size_t operator()(const ClientInfo &client) const
        {
            return hash<int>()(client.fd); // Use fd as the hash key
        }
    };
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
class Socket
{
private:
    // Existing member variables
    const char *socket_path; // Path to the Unix domain socket
    // int socket_fd;          // Socket file descriptor
    int domain;               // Socket domain (AF_UNIX)
    int type;                 // Socket type (SOCK_STREAM)
    int protocol;             // Protocol used (0 for default)
    int max_waiting_requests; // Maximum number of pending requests
    sockaddr_un addr;         // Address for the socket
    int epoll_fd;
    std::mutex epoll_mutex;
    bool epoll_initialized;

public:
    // Existing methods
    Socket();
    void init_epoll(int socket_fd);
    void init_socket_structor();

    int open_socket(); // Open the socket
    int open_socket(int domain, int type, int protocol);
    int close_socket(int client_fd); // Close the socket

    bool bind_server_to_socket(int socket_fd); // Bind the socket to the specified address
    bool start_listening(int socket_fd);
    // Start listening for incoming client connections
    int wait_and_accept(int socket_fd);
    // Wait for a client connection and accept it
    int send_response(int socket_fd, std::vector<int> &resp_vector); // Send a response to the client
    int send_buff(int socket_fd, char *resp_buffer, int length);

    QueryResult get_query(int socket_fd); // Retrieve the query from the client
    QueryResult get_query_with_epoll(int socket_fd, int timeout_ms);
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Client-side functions
    int connect_to_socket(int socket_fd); // Connect to the server socket
    int connect_to_socket(int socket_fd, std::string address);
    bool send_query(int socket_fd, const std::vector<int> &query_vector); // Send a query to the server
    bool send_query(int socket_fd, const std::vector<uint8_t> &query_vector);

    QueryResult get_response(int socket_fd); // Retrieve the response from the server
    QueryResult get_response_with_epoll(int socket_fd, int timeout_ms = 100);

    void printf_vector_bytes(const std::vector<uint8_t> &vec, int length);
    void printf_buffer_bytes(const std::vector<uint8_t> &vec, int length);
    void printf_buffer_bytes(const void *buffer, int length);
};
