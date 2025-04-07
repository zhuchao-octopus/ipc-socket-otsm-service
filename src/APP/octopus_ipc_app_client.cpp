/**
 * @file octopus_ipc_app_client.cpp
 * @brief Client-side implementation for IPC communication via Unix domain socket.
 *
 * This client handles socket creation, connection, sending arithmetic queries,
 * and receiving responses from a server. It supports automatic reconnection
 * in case of unexpected disconnections.
 *
 * Author       : [ak47]
 * Organization : [octopus]
 * Date Time    : [2025/03/13 21:00]
 */

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <list>
#include "octopus_ipc_app_client.hpp"
#include "../IPC/octopus_ipc_ptl.hpp"
#include "../IPC/octopus_ipc_socket.hpp"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Global variables
std::atomic<bool> running{true};    // Controls the receiving loop
std::atomic<int> socket_client{-1}; // Stores socket descriptor
std::thread receiver_thread;        // Thread handling incoming responses
std::mutex callback_mutex;          // Mutex for callback synchronization
Socket client;                      // IPC socket client instance
// std::condition_variable ui_cv;

typedef void (*OctopusAppResponseCallback)(const DataMessage &query_msg, int size);
std::list<OctopusAppResponseCallback> g_callbacks;

// Send a query to the server without additional data
void app_send_query(uint8_t group, uint8_t msg)
{
    app_send_query(group, msg, {});
}

// Send a query to the server with additional data
void app_send_query(uint8_t group, uint8_t msg, const std::vector<uint8_t> &query_array)
{
    if (socket_client.load() < 0)
    {
        std::cerr << "App: Cannot send query, no active connection.\n";
        return;
    }

    DataMessage query_msg;
    query_msg.group = group;
    query_msg.msg = msg;
    query_msg.data = query_array;

    std::vector<uint8_t> serialized_data = query_msg.serialize();
    client.send_query(serialized_data);
}

// Send a command to the server without additional data
void app_send_command(uint8_t group, uint8_t msg)
{
    app_send_command(group, msg, {});
}

// Send a command to the server with additional data
void app_send_command(uint8_t group, uint8_t msg, const std::vector<uint8_t> &query_array)
{
    if (socket_client.load() < 0)
    {
        std::cerr << "App: Cannot send command, no active connection.\n";
        return;
    }

    DataMessage query_msg;
    query_msg.group = group;
    query_msg.msg = msg;
    query_msg.data = query_array;

    std::vector<uint8_t> serialized_data = query_msg.serialize();
    client.send_query(serialized_data);
}
/**
 * @brief Registers a callback function to be invoked upon receiving a response.
 * @param callback Function pointer to the callback function.
 */
void register_ipc_socket_callback(OctopusAppResponseCallback callback)
{
    if (callback)
    {
        std::lock_guard<std::mutex> lock(callback_mutex);
        g_callbacks.push_back(callback);
    }
}
void unregister_ipc_socket_callback(OctopusAppResponseCallback callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex);
    g_callbacks.remove(callback); // list remove，vector erase-remove idiom
}
/**
 * @brief Invokes the registered callback function with the received response data.
 * @param response The received response vector.
 * @param size The size of the response vector.
 */
void notify_response(const DataMessage &query_msg, int size)
{
    std::lock_guard<std::mutex> lock(callback_mutex);
    for (const auto &cb : g_callbacks)
    {
        if (cb)
        {
            cb(query_msg, size);
        }
    }
}

/**
 * @brief Attempts to reconnect to the server if the connection is lost.
 */
void reconnect_to_server()
{
    client.close_socket();
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait before reconnecting

    socket_client.store(client.open_socket(AF_UNIX, SOCK_STREAM, 0));
    socket_client.store(client.connect_to_socket("/tmp/octopus/ipc_socket"));

    if (socket_client.load() < 0)
    {
        std::cerr << "App: Failed to reconnect to the server. Retrying...\n";
    }
    else
    {
        std::cout << "App: Successfully reconnected to the server.\n";
    }
}

// Function to check if data packet is complete
DataMessage is_complete_data_packet(const std::vector<int> &response, int size)
{
    // Create DataMessage object to hold the parsed query data
    DataMessage query_msg;
    query_msg.group = -1;
    query_msg.msg = -1;
    // Ensure that the response is large enough to hold the expected fields
    if (response.size() < 2 || size < 2)
    {
        // If the response is too small, return an empty DataMessage
        return query_msg;  // Ensure a valid return value
    }

    // Parse the group and msg fields from the response
    query_msg.group = static_cast<uint8_t>(response[0]); // Group (message category)
    query_msg.msg = static_cast<uint8_t>(response[1]);   // Msg (command type)

    // Add remaining data elements to the DataMessage's data field
    for (size_t i = 2; i < response.size(); ++i)
    {
        query_msg.data.push_back(static_cast<uint8_t>(response[i]));
    }

    // If the size is valid and there is data, return the constructed DataMessage
    return query_msg;
}

/**
 * @brief Continuously listens for incoming responses from the server.
 * If the connection is lost, the client attempts to reconnect automatically.
 */
void receive_response_loop()
{
    app_send_query(MSG_GROUP_CAR, MSG_GET_INDICATOR_INFO);

    while (running.load()) // Ensure atomic thread-safe check
    {
        if (socket_client.load() < 0)
        {
            std::cerr << "App: No active connection. Attempting to reconnect...\n";
            reconnect_to_server();
            continue;
        }

        auto response_pair = client.get_response();
        std::vector<int> response = response_pair.first;
        int size = response_pair.second;
        DataMessage query_msg = is_complete_data_packet(response, size);
        if (query_msg.group >= 0)
        {
            notify_response(query_msg, size);
        }
        else if (size == 0) // Likely a closed connection
        {
            std::cerr << "App: Connection closed by server. Reconnecting...\n";
            reconnect_to_server();
        }
        else if (size < 0) // size < 0 (error case)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // std::cerr << "App: No data yet, waiting...\n";
            }
            else
            {
                std::cerr << "App: Connection error (errno=" << errno << "). Reconnecting...\n";
                reconnect_to_server();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Prevent excessive CPU usage
    }
}

/**
 * @brief Signal handler for cleaning up resources on interrupt signal (e.g., Ctrl+C).
 * @param signum The received signal number.
 */
void signal_handler(int signum)
{
    std::cout << "Client: Interrupt signal received. Cleaning up...\n";
    exit_cleanup();
    std::exit(signum);
}

/**
 * @brief Initializes the application, sets up the socket connection, and starts the response thread.
 */
__attribute__((constructor)) void app_main()
{
    signal(SIGINT, signal_handler); // Register signal handler for safe shutdown

    socket_client.store(client.open_socket(AF_UNIX, SOCK_STREAM, 0));
    socket_client.store(client.connect_to_socket("/tmp/octopus/ipc_socket"));

    if (socket_client.load() < 0)
    {
        std::cerr << "App: Failed to establish connection with the server.\n";
        client.close_socket();
        // std::exit(-1);
    }
    else
    {
        std::cout << "App: Successfully connected to the server.\n";
    }

    running.store(true);                                  // Set running flag to true
    receiver_thread = std::thread(receive_response_loop); // Start response handling thread
}

/**
 * @brief Cleanup function to properly close the socket and stop the receiving thread.
 */
__attribute__((destructor)) void exit_cleanup()
{
    std::cout << "App: Cleaning up resources...\n";

    if (!std::atomic_exchange(&running, false)) // Ensure cleanup is only performed once
    {
        return;
    }

    client.close_socket();

    if (receiver_thread.joinable())
    {
        receiver_thread.join();
    }
}
