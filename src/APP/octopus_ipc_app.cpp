
/**
 * @file octopus_ipc_app.cpp
 * @brief This file contains the client-side implementation for communicating with a server via a Unix domain socket.
 * It handles socket creation, connection, sending a query (arithmetic operation), and receiving the server's response.
 *
 * The client takes arguments specifying the operation (addition, subtraction, multiplication, or division),
 * followed by two numbers. It then sends this data to the server and prints the result.
 *
 * Author		: [ak47]
 * Organization	: [octopus]
 * Date Time	: [2025/0313/21:00]
 */
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <csignal>
#include "../IPC/octopus_ipc_socket.hpp"
#include "octopus_ipc_app.hpp"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Global variables
int socket_client;
Socket client;
std::atomic<bool> running{true}; // Controls the receiving loop
std::thread receiver_thread;
std::mutex callback_mutex;

typedef void (*ResponseCallback)(const std::vector<int> &response, int size);
ResponseCallback g_callback = nullptr;

/**
 * @brief Signal handler to clean up resources when an interrupt signal (e.g., Ctrl+C) is received.
 * @param signum The signal number.
 */
void signal_handler(int signum)
{
    std::cout << "Client: Interrupt signal received. Cleaning up...\n";
    running = false; // Stop the receiving thread
    exit_cleanup();  // Ensure graceful cleanup
    std::exit(signum);
}

/**
 * @brief Registers a callback function to be invoked when a response is received.
 * @param callback Function pointer to the callback.
 */
void register_ipc_socket_callback(ResponseCallback callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex);
    g_callback = callback;
}

/**
 * @brief Notifies the UI layer by invoking the registered callback.
 * @param response The received response data.
 * @param size The size of the response vector.
 */
void notify_response(const std::vector<int> &response, int size)
{
    std::lock_guard<std::mutex> lock(callback_mutex);
    if (g_callback)
    {
        g_callback(response, size);
    }
}

/**
 * @brief Sends a query request to the server.
 * @param operation The arithmetic operation.
 * @param number1 The first number.
 * @param number2 The second number.
 */
void process_query(int operation, int number1, int number2)
{
    std::vector<int> query_vector = {operation, number1, number2};
    client.send_query(query_vector);
}

/**
 * @brief Runs the response receiving loop in a separate thread.
 * Continuously listens for incoming data and invokes the callback when a response is received.
 */
void receive_response_loop()
{
    std::vector<int> query_vector = {11, 101, 0};
    client.send_query(query_vector);
    while (running)
    {
        std::pair<std::vector<int>, int> response_pair = client.get_response();
        std::vector<int> response = response_pair.first;
        int size = response_pair.second;
        std::cout << "App: Received response: ";
        if (size > 0)
        {
            //client.printf_vector_bytes(response, size);
            notify_response(response, size);
        }
    }
}

/**
 * @brief Application entry point. Called when the shared library is loaded.
 * Sets up the socket connection and starts the response receiving thread.
 */
__attribute__((constructor)) void app_main()
{
    // Register signal handler for cleanup on exit
    signal(SIGINT, signal_handler);

    // Open and connect the socket
    socket_client = client.open_socket(AF_UNIX, SOCK_STREAM, 0);
    socket_client = client.connect_to_socket("/tmp/octopus/ipc_socket");

    if (socket_client < 0)
    {
        std::cerr << "App: Invalid socket connection" << std::endl;
        client.close_socket();
        std::exit(-1);
    }
    else
    {
        std::cout << "App: Opened a socket [" << socket_client << "] connected to the server" << std::endl;
    }
    running = true;
    // Start a separate thread to handle response reception
    receiver_thread = std::thread(receive_response_loop);
}

/**
 * @brief Cleanup function. Called when the shared library is unloaded.
 * Ensures proper shutdown of the socket and receiving thread.
 */
__attribute__((destructor)) void exit_cleanup()
{
    std::cout << "App: exit_cleanup called closing socket.\n";
    running = false; // Stop the receiving thread
    client.close_socket();

    if (receiver_thread.joinable())
    {
        receiver_thread.join(); // Ensure safe thread exit
    }
}
