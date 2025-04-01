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
#include <dlfcn.h>
#include "octopus_ipc_socket.hpp"
#include "../OTSM/octopus_carinfor.h"
#include "../OTSM/octopus_ipc_socket.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////

typedef void (*TaskManagerStateRegistCallbackFunc)(CarInforCallback_t callback);
typedef void (*TaskManagerStateStopRunningFunc)();
typedef int (*TaskManagerStateDoCommandFunc)(uint8_t *, uint8_t);

TaskManagerStateStopRunningFunc taskManagerStateStopRunning = NULL;
TaskManagerStateDoCommandFunc taskManagerStateDoCommand = NULL;
TaskManagerStateRegistCallbackFunc taskManagerStateRegistCallbackFunc = NULL;

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

int handle_calculation(int client_fd, const std::vector<int> &query_vector);
int handle_car_infor(int client_fd, const std::vector<int> &query_vector);
int handle_help(int client_fd, const std::vector<int> &query_vector);
// Path for the IPC socket file

const char *socket_path = "/tmp/octopus/ipc_socket";

// Server object to handle socket operations
Socket server;

// Mutex for server operations to ensure thread-safety
std::mutex server_mutex;

// Unordered set to track active clients
std::unordered_set<int> active_clients;

// Mutex for client operations to ensure thread-safety
std::mutex clients_mutex;

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
    std::cout << "Server handling otsm message cmd_parameter=" << cmd_parameter << std::endl;
  
#if 1
    ///std::vector<std::thread> threads;  
    for (int client_id : active_clients) 
    {
        ///std::thread notify_thread(notify_carInfor_to_client, client_id, cmd_parameter);
        ///threads.push_back(std::move(notify_thread));  
        notify_carInfor_to_client(client_id, cmd_parameter);
    }

    ///for (auto& thread : threads)
    ///{
    ///    thread.join();  
    ///}
#endif
}

int main()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    initialize_otsm();
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
    if (taskManagerStateStopRunning)
        taskManagerStateStopRunning();

    return 0;
}

// Function to handle communication with a specific client
void handle_client(int client_fd)
{
    std::cout << "Server handling client [" << client_fd << "]." << std::endl;

    // Buffers to hold query and response data
    std::vector<int> query_vector(3);
    int handle_result = 0;
    // Loop to continuously process client queries
    while (true)
    {
        // Lock the server mutex to safely fetch the query data
        {
            // std::lock_guard<std::mutex> lock(server_mutex);
            query_vector = server.get_query(client_fd);
        }

        // If the query is empty, break the loop and end the communication
        if (query_vector.empty())
        {
            /// std::cout << "Server handling client [" << client_fd << "] empty,existing." << std::endl;
            break;
        }

        // Perform the requested operation based on the query
        switch (query_vector[0])
        {
        case 0:
            handle_result = handle_help(client_fd, query_vector);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            handle_result = handle_calculation(client_fd, query_vector);
            break;
        case 11:
            handle_result = handle_car_infor(client_fd, query_vector);
            break;
        default:
            handle_result = handle_help(client_fd, query_vector);
            break;
        }

        std::cout << "Server handling client [" << client_fd << "] cmd[" << query_vector[0] << "] done." << std::endl;
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

int handle_help(int client_fd, const std::vector<int> &query_vector)
{
    std::vector<int> resp_vector(1);
    /// std::cout << "Server handle_help"<< std::endl;
    for (int client_id : active_clients) // 遍历所有活跃客户端 ID
    {
        std::cout << "Server connected clients [" << client_fd << "] ";
    }
    std::cout << std::endl;
    // Set the result in the response vector
    resp_vector[0] = CMD_GET_HELP_INFO;
    // Lock the server mutex to safely send the response to the client
    {
        std::lock_guard<std::mutex> lock(server_mutex);
        server.send_response(client_fd, resp_vector);
    }
    return 0;
}
// Function to handle calculation logic
int handle_calculation(int client_fd, const std::vector<int> &query_vector)
{
    int calc_result = 0;
    std::vector<int> resp_vector(1);

    switch (query_vector[0])
    {
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
        if (query_vector[2] == 0)
        {
            std::cerr << "Server Error: Division by zero!" << std::endl;
            calc_result = 0;
        }
        else
        {
            calc_result = query_vector[1] / query_vector[2];
        }
        break;
    default:
        std::cerr << "Server Invalid operation requested!" << std::endl;
        break;
    }
    // Set the calculated result in the response vector
    resp_vector[0] = calc_result;
    // Lock the server mutex to safely send the response to the client
    {
        std::lock_guard<std::mutex> lock(server_mutex);
        server.send_response(client_fd, resp_vector);
    }
    return calc_result;
}

int handle_car_infor(int client_fd, const std::vector<int> &query_vector)
{
    notify_carInfor_to_client(client_fd, query_vector[1]);
    return 0;
}

void notify_carInfor_to_client(int client_fd, int cmd)
{
    // std::vector<int> resp_vector(sizeof(indicator));
    switch (cmd)
    {
    case CMD_GET_INDICATOR_INFO:
    {
        carinfo_indicator_t *carinfo_indicator = get_indicator_info();
        if (carinfo_indicator == nullptr)
        {
            // LOG_LEVEL("get_indicator_info() returned nullptr");
            break;
        }
        //  Lock the server mutex to safely send the response to the client
        std::cout << "Server handling client [" << client_fd << "] handle_car_infor " << sizeof(carinfo_indicator_t) << " bytes:";
        server.printf_buffer_bytes(carinfo_indicator, sizeof(carinfo_indicator_t));

        {
            std::lock_guard<std::mutex> lock(server_mutex);
            server.send_buff(client_fd, reinterpret_cast<char *>(carinfo_indicator), sizeof(carinfo_indicator_t));
        }
        break;
    }
    case CMD_GET_METER_INFO:
    {
        carinfo_meter_t *carinfo_meter = get_meter_info();
        if (carinfo_meter == nullptr)
        {
            // LOG_LEVEL("get_indicator_info() returned nullptr");
            break;
        }
        //  Lock the server mutex to safely send the response to the client
        std::cout << "Server handling client [" << client_fd << "] handle_car_infor " << sizeof(carinfo_meter_t) << " bytes:";
        server.printf_buffer_bytes(carinfo_meter, sizeof(carinfo_meter_t));

        {
            std::lock_guard<std::mutex> lock(server_mutex);
            server.send_buff(client_fd, reinterpret_cast<char *>(carinfo_meter), sizeof(carinfo_meter_t));
        }
        break;
    }
    case CMD_GET_DRIVINFO_INFO:
    {
        carinfo_drivinfo_t *carinfo_drivinfo = get_drivinfo_info();
        if (carinfo_drivinfo == nullptr)
        {
            // LOG_LEVEL("get_indicator_info() returned nullptr");
            break;
        }

        //  Lock the server mutex to safely send the response to the client
        std::cout << "Server handling client [" << client_fd << "] handle_car_infor " << sizeof(carinfo_drivinfo_t) << " bytes:";
        server.printf_buffer_bytes(carinfo_drivinfo, sizeof(carinfo_drivinfo_t));
        {
            std::lock_guard<std::mutex> lock(server_mutex);
            server.send_buff(client_fd, reinterpret_cast<char *>(carinfo_drivinfo), sizeof(carinfo_drivinfo_t));
        }
        break;
    }
    default:
    handle_help(client_fd,{0});
        break;
    }
}
