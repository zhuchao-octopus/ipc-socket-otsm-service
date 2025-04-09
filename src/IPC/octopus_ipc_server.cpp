/*////////////////////////////////////////////////////////////////////////////////////////////////////
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
//////////////////////////////////////////////////////////////////////////////////////////////////////
#include <mutex>
#include <atomic>
#include <unordered_set>
#include <algorithm>
#include <dlfcn.h>
#include "octopus_ipc_socket.hpp"
#include "octopus_logger.hpp"
#include "../OTSM/octopus_carinfor.h"
#include "../OTSM/octopus_ipc_socket.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////
typedef int (*TaskManagerStateDoCommandFunc)(uint8_t *, uint8_t);

typedef void (*TaskManagerStateRegistCallbackFunc)(CarInforCallback_t callback);
typedef void (*TaskManagerStateStopRunningFunc)();

typedef void (*TaskManagerState_set_message_push_delay)(uint16_t delay_ms);

TaskManagerStateStopRunningFunc taskManagerStateStopRunning = NULL;
TaskManagerStateDoCommandFunc taskManagerStateDoCommand = NULL;
TaskManagerStateRegistCallbackFunc taskManagerStateRegistCallbackFunc = NULL;

TaskManagerState_set_message_push_delay ostsm_set_message_push_delay = NULL;

typedef carinfo_meter_t *(*otsm_get_meter_info)();
typedef carinfo_indicator_t *(*otsm_get_indicator_info)();
typedef carinfo_drivinfo_t *(*otsm_get_drivinfo_info)();

otsm_get_meter_info get_meter_info = NULL;
otsm_get_indicator_info get_indicator_info = NULL;
otsm_get_drivinfo_info get_drivinfo_info = NULL;

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to handle client communication
void CarInforNotify_Callback(int cmd_parameter);
void notify_carInfor_to_client(int client_fd, int cmd);
void handle_client(int client_fd);

int handle_calculation(int client_fd, const DataMessage &query_msg);
int handle_car_infor(int client_fd, const DataMessage &query_msg);
int handle_help(int client_fd, const DataMessage &query_msg);
int handle_config(int client_fd, const DataMessage &query_msg);
// Path for the IPC socket file

const char *socket_path = "/tmp/octopus/ipc_socket";

// Server object to handle socket operations
Socket server;

// Mutex for server operations to ensure thread-safety
std::mutex server_mutex;

// Mutex for client operations to ensure thread-safety
std::mutex clients_mutex;

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
// Thread-safe unordered set for active clients
std::unordered_set<ClientInfo> active_clients;

// **Thread-Safe Functions**
void add_client(int fd, const std::string &ip, bool flag)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    active_clients.insert(ClientInfo(fd, ip, flag));
}

void remove_client(int fd)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto it = active_clients.begin(); it != active_clients.end(); ++it)
    {
        if (it->fd == fd)
        {
            active_clients.erase(it);
            break;
        }
    }
}

void print_active_clients()
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto &client : active_clients)
    {
        std::cout << "Server client fd: " << client.fd
                  << ", ip: " << client.ip
                  << ", flag: " << client.flag << std::endl;
    }
}

void update_client(int fd, bool new_flag)
{
    std::lock_guard<std::mutex> lock(clients_mutex); // 线程安全

    // 在集合中查找匹配的 `fd`
    auto it = std::find_if(active_clients.begin(), active_clients.end(),
                           [fd](const ClientInfo &client)
                           { return client.fd == fd; });

    if (it != active_clients.end())
    {
        // 先取出原始值，修改后重新插入
        ClientInfo updated_client = *it;
        updated_client.flag = new_flag;

        active_clients.erase(it);                         // 删除旧的
        active_clients.insert(std::move(updated_client)); // 插入更新后的
    }
    else
    {
        std::cerr << "Client FD not found: " << fd << std::endl;
    }
}
void update_client(int fd, const std::string &ip)
{
    std::lock_guard<std::mutex> lock(clients_mutex); // 线程安全

    // 在集合中查找匹配的 `fd`
    auto it = std::find_if(active_clients.begin(), active_clients.end(),
                           [fd](const ClientInfo &client)
                           { return client.fd == fd; });

    if (it != active_clients.end())
    {
        // 先取出原始值，修改后重新插入
        ClientInfo updated_client = *it;
        updated_client.ip = ip;

        active_clients.erase(it);                         // 删除旧的
        active_clients.insert(std::move(updated_client)); // 插入更新后的
    }
    else
    {
        std::cerr << "Client FD not found: " << fd << std::endl;
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to ensure the directory for the socket file exists
void ensure_directory_exists(const char *path)
{
    std::string dir = path;
    size_t pos = dir.find_last_of('/');

    if (pos != std::string::npos)
    {
        std::string parent_dir = dir.substr(0, pos);
        struct stat st;

        // If the parent directory does not exist, create it
        if (stat(parent_dir.c_str(), &st) == -1)
        {
            if (mkdir(parent_dir.c_str(), 0777) == -1)
            {
                std::cerr << "Failed to create directory: " << strerror(errno) << std::endl;
            }
        }
    }
}

// Function to remove the old socket file if it exists
void remove_old_socket()
{
    unlink(socket_path);
}

// Signal handler for clean-up on interrupt (e.g., Ctrl+C)
void signal_handler(int signum)
{
    std::cout << "Server Interrupt signal received. Cleaning up...\n";
    server.close_socket();
    if (taskManagerStateStopRunning)
        taskManagerStateStopRunning();
    exit(signum);
}

void initialize_server()
{
    std::cout << "Server initialize server started." << std::endl;
    signal(SIGPIPE, SIG_IGN);
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
    // server.query_buffer_size = 20;
    // server.respo_buffer_size = 20;

    std::cout << "Server waiting for client connections..." << std::endl;
}

void initialize_otsm()
{
    // 加载共享库
    std::cout << "Server initialize otsm started." << std::endl;
    void *handle = dlopen("libOTSM.so", RTLD_LAZY);
    if (!handle)
    {
        std::cerr << "Server Failed to load otsm library: " << dlerror() << std::endl;
        return;
    }
    /// std::cout << "Server sucecess to load otsm library: " << dlerror() << std::endl;
    ///  你可以在这里调用 OTSM 库中的函数，假设它有一个初始化函数 `initialize`。
    ///  例如，假设 OTSM 库有一个 C 风格的 `initialize` 函数

    taskManagerStateStopRunning = (TaskManagerStateStopRunningFunc)dlsym(handle, "TaskManagerStateStopRunning");

    if (!taskManagerStateStopRunning)
    {
        std::cerr << "Server Failed to find initialize function: " << dlerror() << std::endl;
        dlclose(handle);
        return;
    }

    taskManagerStateDoCommand = (TaskManagerStateDoCommandFunc)dlsym(handle, "ipc_socket_doCommand");
    if (!taskManagerStateDoCommand)
    {
        std::cerr << "Server Failed to find taskManagerStateDoCommand: " << dlerror() << std::endl;
        dlclose(handle);
        return;
    }

    taskManagerStateRegistCallbackFunc = (TaskManagerStateRegistCallbackFunc)dlsym(handle, "register_car_infor_callback");
    if (!taskManagerStateRegistCallbackFunc)
    {
        std::cerr << "Server Failed to find taskManagerStateRegistCallbackFunc: " << dlerror() << std::endl;
        dlclose(handle);
        return;
    }

    ostsm_set_message_push_delay = (TaskManagerState_set_message_push_delay)dlsym(handle, "set_message_push_delay");
    if (!ostsm_set_message_push_delay)
    {
        std::cerr << "Server Failed to find ostsm_set_message_push_delay: " << dlerror() << std::endl;
        dlclose(handle);
        return;
    }

    get_meter_info = (carinfo_meter_t * (*)()) dlsym(handle, "app_carinfo_get_meter_info");
    if (!get_meter_info)
    {
        std::cerr << "Server Failed to find get_meter_info: " << dlerror() << std::endl;
        dlclose(handle);
        return;
    }
    get_indicator_info = (carinfo_indicator_t * (*)()) dlsym(handle, "app_carinfo_get_indicator_info");
    if (!get_indicator_info)
    {
        std::cerr << "Server Failed to find get_indicator_info: " << dlerror() << std::endl;
        dlclose(handle);
        return;
    }

    get_drivinfo_info = (carinfo_drivinfo_t * (*)()) dlsym(handle, "app_carinfo_get_drivinfo_info");
    if (!get_drivinfo_info)
    {
        std::cerr << "Server Failed to find get_drivinfo_info: " << dlerror() << std::endl;
        dlclose(handle);
        return;
    }

    taskManagerStateRegistCallbackFunc(CarInforNotify_Callback);
    /// 调用库中的初始化函数
    /// initialize_func();
}

void CarInforNotify_Callback(int cmd_parameter)
{
    // std::cout << "Server handling otsm message cmd_parameter=" << cmd_parameter << std::endl;
    for (const auto &client : active_clients)
    {
        /// std::thread notify_thread(notify_carInfor_to_client, client_id, cmd_parameter);
        /// threads.push_back(std::move(notify_thread));
        if (client.flag) // need push callback
            notify_carInfor_to_client(client.fd, cmd_parameter);
    }
}

int main()
{
    LOG_CC("\n#######################################################################################");
    LOG_CC("Octopus IPC Socket Server Started Successfully.");
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    initialize_otsm();
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait before reconnecting
    initialize_server();

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main loop to accept and handle client connections
    while (true)
    {
        int client_fd = server.wait_and_accept();

        // If accepting a client fails, continue to the next iteration
        if (client_fd < 0)
        {
            std::cerr << "Server Failed to accept client connection" << std::endl;
            continue;
            // break; for test
        }

        // Lock the clients mutex to safely modify the active clients set
        {
            /// std::lock_guard<std::mutex> lock(clients_mutex);
            /// active_clients.insert(client_fd);
            add_client(client_fd, "", false);
        }

        // Create a thread to handle the client communication
        std::thread client_thread(handle_client, client_fd);

        // Detach the thread so it runs independently
        client_thread.detach();
    }

    // Close the server socket before exiting
    server.close_socket();
    if (taskManagerStateStopRunning)
        taskManagerStateStopRunning();

    return 0;
}

// Function to handle communication with a specific client
/**
 * @brief Handles communication with a connected client socket.
 *
 * This function enters an infinite loop to continuously read and process queries
 * from the connected client via the server's query interface. It parses the incoming
 * messages according to the protocol, dispatches them to the appropriate handler functions,
 * and gracefully shuts down the connection upon disconnection or error.
 *
 * @param client_fd The file descriptor of the connected client socket.
 */
void handle_client(int client_fd)
{
    std::cout << "Server handling client connection [" << client_fd << "]..." << std::endl;

    int handle_result = 0;

    QueryResult query_result;
    DataMessage query_msg;

    // Main processing loop: handle client queries continuously
    while (true)
    {
        // Try to fetch a query from the client using a thread-safe interface
        query_result = server.get_query(client_fd);

        // Check the status of the query attempt
        switch (query_result.status)
        {
        case QueryStatus::Timeout:
            // No data received within timeout, continue waiting
            //std::cout << "Server No data from client yet, waiting again..." << std::endl;
            continue;

        case QueryStatus::Success:
            // Data received, proceed to process it
            break;

        case QueryStatus::Disconnected:
            // Client disconnected, exit loop and clean up
            std::cout << "Server client [" << client_fd << "] disconnected." << std::endl;
            [[fallthrough]];

        case QueryStatus::Error:
        default:
            // Any other error or invalid socket state, terminate connection
            std::cerr << "Server connection for client [" << client_fd << "] closing." << std::endl;
            goto cleanup;
        }

        // Verify the minimal size of a valid query message
        if (query_result.data.size() < 5)
        {
            std::cerr << "Server invalid query size from client [" << client_fd << "]." << std::endl;
            continue;
        }

        // Clear previous query_msg data and parse new message fields
        query_msg = DataMessage();
        query_msg.group = query_result.data[2];  // Group: functional category
        query_msg.msg = query_result.data[3];    // Msg: specific command ID
        query_msg.length = query_result.data[4]; // Length: expected data length

        // Copy the actual payload starting from byte index 5
        query_msg.data.assign(query_result.data.begin() + 5, query_result.data.end());

        // Validate the packet before processing
        if (!query_msg.is_valid_packet())
        {
            std::cerr << "Server Invalid packet received from client [" << client_fd << "]." << std::endl;
            continue;
        }

        // Dispatch to the appropriate handler based on group ID
        switch (query_msg.group)
        {
        case MSG_GROUP_HELP:
            handle_result = handle_help(client_fd, query_msg); // Help/info request
            break;

        case MSG_GROUP_SET:
            handle_result = handle_config(client_fd, query_msg); // Configuration command
            break;

        case 2:
        case 3:
        case 4:
            handle_result = handle_calculation(client_fd, query_msg); // Placeholder groups
            break;

        case MSG_GROUP_CAR:
            handle_result = handle_car_infor(client_fd, query_msg); // Vehicle info commands
            break;

        default:
            // Unknown group, fallback to help
            handle_result = handle_help(client_fd, query_msg);
            break;
        }

        // Log success after handling the message
        std::cout << "Server handled client [" << client_fd << "] "
                  << "[Group: " << static_cast<int>(query_msg.group) << "] "
                  << "[Msg: " << static_cast<int>(query_msg.msg) << "] done." << std::endl;
    }

cleanup:

    // Gracefully close the client socket and remove from active list
    close(client_fd);
    remove_client(client_fd);

    std::cout << "Server connection for client [" << client_fd << "] closed." << std::endl;
}

int handle_help(int client_fd, const DataMessage &query_msg)
{
    // Print the parsed DataMessage for debugging purposes
    query_msg.print("Server help"); // Print the incoming query message for visibility

    // Prepare response vector with a predefined response code
    std::vector<int> resp_vector(1);

    // Optionally print active clients to log the current client activity
    print_active_clients();
    /// std::cout << std::endl;
    // Set the response message based on the protocol
    resp_vector[0] = MSG_GROUP_HELP; // Respond with predefined help information message

    // Lock the server mutex to safely send the response to the client
    {
        // Ensure thread safety while sending the response back to the client
        std::lock_guard<std::mutex> lock(server_mutex);
        server.send_response(client_fd, resp_vector); // Send the help info response to client
    }

    // Return success
    return 0;
}

int handle_config(int client_fd, const DataMessage &query_msg)
{
    // Extract the file descriptor from the query data or use the provided one
    int fd = (query_msg.data.empty() || query_msg.data[0] <= 0) ? client_fd : query_msg.data[0];

    // If the message is related to IPC socket configuration, update the client status
    if (query_msg.msg == MSG_IPC_SOCKET_CONFIG_FLAG)
    {
        // Update client based on the first data value
        bool is_active = ((query_msg.data.size() >= 2) && (query_msg.data[1] > 0));
        update_client(fd, is_active); // Update the client state (active/inactive)
    }
    else if (query_msg.msg == MSG_IPC_SOCKET_CONFIG_PUSH_DELAY)
    {
        if (ostsm_set_message_push_delay)
            ostsm_set_message_push_delay(MERGE_BYTES(query_msg.data[0], query_msg.data[1]));
    }
    else if (query_msg.msg == MSG_IPC_SOCKET_CONFIG_IP)
    {
        // Update client based on the first data value
        bool is_active = ((query_msg.data.size() >= 2) && (query_msg.data[1] > 0));
        update_client(fd, std::string(query_msg.data.begin(), query_msg.data.end())); // Update the client state (active/inactive)
    }

    // Log the current active clients
    print_active_clients();
    std::cout << std::endl;

    // Prepare response vector with the set message group
    std::vector<int> resp_vector(1, MSG_GROUP_SET); // Set response to MSG_GROUP_SET

    // Lock the server mutex to safely send the response to the client
    {
        std::lock_guard<std::mutex> lock(server_mutex);
        server.send_response(client_fd, resp_vector); // Send the response to the client
    }

    // Return success
    return 0;
}

// Function to handle calculation logic
int handle_calculation(int client_fd, const DataMessage &query_msg)
{
    int calc_result = 0;
    std::vector<int> resp_vector(1); // Initialize response vector with one element

    // Ensure the data is large enough to hold the operands
    if (query_msg.data.size() < 3)
    {
        std::cerr << "Server Error: Insufficient operands for calculation!" << std::endl;
        resp_vector[0] = -1; // Indicate error with a negative result
    }
    else
    {
        // Switch operation based on the first data element (operation type)
        switch (query_msg.data[0])
        {
        case 1: // Addition
            calc_result = query_msg.data[1] + query_msg.data[2];
            break;
        case 2: // Subtraction
            calc_result = query_msg.data[1] - query_msg.data[2];
            break;
        case 3: // Multiplication
            calc_result = query_msg.data[1] * query_msg.data[2];
            break;
        case 4: // Division
            if (query_msg.data[2] == 0)
            {
                std::cerr << "Server Error: Division by zero!" << std::endl;
                calc_result = 0; // Set result to 0 in case of division by zero
            }
            else
            {
                calc_result = query_msg.data[1] / query_msg.data[2];
            }
            break;
        default:
            std::cerr << "Server Error: Invalid operation requested!" << std::endl;
            calc_result = -1; // Indicate error with a negative result
            break;
        }

        // Set the calculated result in the response vector
        resp_vector[0] = calc_result;
    }

    // Lock the server mutex to safely send the response to the client
    {
        std::lock_guard<std::mutex> lock(server_mutex);
        server.send_response(client_fd, resp_vector);
    }

    return calc_result;
}

int handle_car_infor(int client_fd, const DataMessage &query_msg)
{
    notify_carInfor_to_client(client_fd, query_msg.msg);
    return 0;
}
// Helper function to handle the car info response logic
template <typename T>
void send_car_info_to_client(int client_fd, int msg, T *car_info, size_t size, const std::string &info_type)
{
    if (car_info == nullptr)
    {
        std::cerr << "Server Error: " << info_type << " returned nullptr!" << std::endl;
        return;
    }

    // Create a DataMessage to follow the protocol format
    DataMessage data_msg;
    data_msg.group = MSG_GROUP_CAR; // Set appropriate group based on the info type
    data_msg.msg = msg;             // Set message ID based on info type
    data_msg.length = size;
    // Directly copy the car info data into the msg.data vector
    data_msg.data.clear(); // Clear any existing data
    data_msg.data.insert(data_msg.data.end(), reinterpret_cast<uint8_t *>(car_info), reinterpret_cast<uint8_t *>(car_info) + size);

    // Serialize the DataMessage into the protocol format
    std::vector<uint8_t> serialized_data = data_msg.serialize();
    size_t data_size = serialized_data.size();
    char *buffer = reinterpret_cast<char *>(serialized_data.data());

    // Print the buffer contents for debugging
    std::cout << "Server handling client [" << client_fd << "] " << info_type << " " << data_size << " bytes: ";
    server.printf_buffer_bytes(buffer, data_size);
    // Lock the server mutex to safely send the response to the client
    {
        std::lock_guard<std::mutex> lock(server_mutex);
        server.send_buff(client_fd, buffer, data_size);
    }
}

// Main function to notify car info to the client
void notify_carInfor_to_client(int client_fd, int cmd)
{
    switch (cmd)
    {
    case CMD_GET_INDICATOR_INFO:
    {
        carinfo_indicator_t *carinfo_indicator = get_indicator_info();
        send_car_info_to_client(client_fd, cmd, carinfo_indicator, sizeof(carinfo_indicator_t), "handle_car_infor (Indicator)");
        break;
    }
    case CMD_GET_METER_INFO:
    {
        carinfo_meter_t *carinfo_meter = get_meter_info();
        send_car_info_to_client(client_fd, cmd, carinfo_meter, sizeof(carinfo_meter_t), "handle_car_infor (Meter)");
        break;
    }
    case CMD_GET_DRIVINFO_INFO:
    {
        carinfo_drivinfo_t *carinfo_drivinfo = get_drivinfo_info();
        send_car_info_to_client(client_fd, cmd, carinfo_drivinfo, sizeof(carinfo_drivinfo_t), "handle_car_infor (Driver)");
        break;
    }
    default:
        // If the command is invalid, call handle_help to notify the client
        // handle_help(client_fd, {0});
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
