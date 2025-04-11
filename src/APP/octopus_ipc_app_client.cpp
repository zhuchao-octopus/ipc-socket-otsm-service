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
#include <algorithm> // For std::remove_if
#include <list>
#include "octopus_ipc_app_client.hpp"
#include "../IPC/octopus_ipc_socket.hpp"
#include "../IPC/octopus_ipc_threadpool.hpp" // 引入线程池头文件
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ipc_redirect_log_to_file();

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const std::string ipc_server_path_name = "/res/bin/octopus_ipc_server";
const std::string ipc_server_name = "octopus_ipc_server";
const std::string ipc_socket_path_name = "/tmp/octopus/ipc_socket";

// Global variables
std::atomic<bool> socket_running{true}; // Controls the receiving loop
std::atomic<int> socket_client{-1};     // Stores socket descriptor
std::thread ipc_receiver_thread;        // Thread handling incoming responses
std::mutex callback_mutex;              // Mutex for callback synchronization
Socket client;                          // IPC socket client instance

// Initialize the global thread pool object
OctopusThreadPool g_threadPool(4, 100, TaskOverflowStrategy::DropOldest);

bool request_push_data = false;

struct CallbackEntry
{
    std::string func_name;
    OctopusAppResponseCallback cb;
    int failure_count = 0;
};

std::list<CallbackEntry> g_named_callbacks;

// Create an instance of the message bus
OctopusMessageBus messageBus;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool file_exists_and_executable(const std::string &path)
{
    return access(path.c_str(), X_OK) == 0;
}
bool is_process_running(const std::string &process_name)
{
    FILE *fp = popen(("pidof " + process_name).c_str(), "r");
    if (!fp)
        return false;

    char buffer[128];
    bool found = fgets(buffer, sizeof(buffer), fp) != nullptr;
    pclose(fp);
    return found;
}

bool is_ipc_socket_server_process_running(const std::string &process_name)
{
    FILE *fp;
    const size_t buffer_size = 500; // Increase buffer size to accommodate larger output
    char buffer[buffer_size];       // Buffer to store process output
    bool found = false;

    // Open a pipe to run the ps command and get all processes
    fp = popen("ps aux", "r");
    if (fp == nullptr)
    {
        std::cerr << "Failed to run ps command" << std::endl;
        return false;
    }

    // Read the output of ps command in chunks (buffer_size at a time)
    while (fread(buffer, 1, buffer_size, fp) > 0)
    {
        // Check the chunk for the process name, making sure to exclude the 'grep' command itself
        if (strstr(buffer, process_name.c_str()) != nullptr && strstr(buffer, "grep") == nullptr)
        {
            found = true;
            break;
        }
    }

    fclose(fp);
    return found;
}

void start_process_as_server(const std::string &process_path)
{
    // Check if the process path exists
    struct stat buffer;
    if (stat(process_path.c_str(), &buffer) != 0)
    {
        // If the process path does not exist, print an error and return
        std::cerr << "Client: Process path does not exist: " << process_path << std::endl;
        return;
    }
    // Construct the shell command to start the process in the background
    // Redirecting both stdout and stderr to the log file
    // std::string command = process_path + " >> " + log_file + " 2>&1 &";
    // Construct the shell command to start the process in the background with output redirection
    // std::string command = "nohup " + process_path + " >> " + log_file + " 2>&1 &";
    std::string command = process_path + " >> " + log_file + " 2>&1 &";
    // Print the command to debug
    std::cout << "Client: Command to run - " << command << std::endl;
    // Execute the system command to start the process
    int result = system(command.c_str());

    // Check the result of the system command to see if the process was started successfully
    if (result == -1)
    {
        // If the system call fails, print an error message
        std::cout << "Client: Failed to IPC Socket Server start process: " << process_path << std::endl;
    }
    else
    {
        // If the system call succeeds, print a success message
        std::cout << "Client: IPC Socket Server process started: " << process_path << std::endl;
        // Optionally, check if the process is running after a short delay
        // sleep(1);                                                        // Wait for the process to start
        // std::string ps_command = "ps aux | grep '" + process_path + "'"; // Using 'grep' to filter the output
        // system(ps_command.c_str());                                      // Execute the ps command to check if the process is running
        if (!is_ipc_socket_server_process_running(process_path))
        {
            std::cout << "Client: Bad Failed to start IPC Socket Server,restart " << process_path << std::endl;
        }
    }
}

void ipc_init_threadpool()
{
    // g_threadPool = std::make_unique<OctopusThreadPool>(4, 100);
    // g_threadPool.reset();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
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
    query_msg.length = query_msg.data.size();

    query_msg.printMessage("Send query");
    std::vector<uint8_t> serialized_data = query_msg.serializeMessage();
    client.send_query(socket_client.load(), serialized_data);
}

// Send a command to the server without additional data
void app_send_command(uint8_t group, uint8_t msg)
{
    app_send_command(group, msg, {});
}

// Send a command to the server with additional data
void app_send_command(uint8_t group, uint8_t msg, const std::vector<uint8_t> &parameters)
{
    if (socket_client.load() < 0)
    {
        std::cerr << "App: Cannot send command, no active connection.\n";
        return;
    }

    DataMessage query_msg;
    query_msg.group = group;
    query_msg.msg = msg;
    query_msg.data = parameters;

    std::vector<uint8_t> serialized_data = query_msg.serializeMessage();
    client.send_query(socket_client.load(), serialized_data);
}
/**
 * @brief Registers a callback function to be invoked upon receiving a response.
 * @param callback Function pointer to the callback function.
 */
void register_ipc_socket_callback(std::string func_name, OctopusAppResponseCallback callback)
{
    if (callback)
    {
        std::lock_guard<std::mutex> lock(callback_mutex);
        g_named_callbacks.push_back({func_name, callback});
        LOG_CC("App: Registered callback: " + func_name);
    }
}

/**
 * @brief Unregisters a previously registered IPC callback function.
 *
 * This function safely removes a callback from the global callback list
 * by comparing function pointer addresses. It uses a mutex to ensure thread safety.
 * Additionally, logs the function name and pointer address of the callback being unregistered.
 *
 * @param callback The function pointer of the callback to be unregistered.
 */
void unregister_ipc_socket_callback(OctopusAppResponseCallback callback)
{
    // Lock the callback list to ensure thread safety
    std::lock_guard<std::mutex> lock(callback_mutex);

    // Use std::remove_if to find all entries matching the given callback
    auto it = std::remove_if(g_named_callbacks.begin(), g_named_callbacks.end(),
                             [callback](const CallbackEntry &entry)
                             {
                                 return entry.cb == callback;
                             });

    if (it != g_named_callbacks.end())
    {
        // Log each unregistered callback's name and address
        for (auto iter = it; iter != g_named_callbacks.end(); ++iter)
        {
            std::ostringstream oss;
            oss << "App: Unregistered callback: name=" << iter->func_name
                << ", address=" << reinterpret_cast<const void *>(iter->cb);
            LOG_CC(oss.str());
        }

        // Erase all matching entries from the callback list
        g_named_callbacks.erase(it, g_named_callbacks.end());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Invokes the registered callback function with the received response data.
 * @param response The received response vector.
 * @param size The size of the response vector.
 */
void invoke_notify_response(const DataMessage &query_msg, int size)
{
    // 复制一份有效回调（值拷贝或智能指针）
    std::vector<std::shared_ptr<CallbackEntry>> active_callbacks;

    {
        std::lock_guard<std::mutex> lock(callback_mutex);
        for (auto &entry : g_named_callbacks)
        {
            active_callbacks.emplace_back(std::make_shared<CallbackEntry>(entry)); // 深拷贝或用智能指针
        }
    }

    constexpr int FAILURE_THRESHOLD = 3;

    for (auto &entry_ptr : active_callbacks)
    {
        // 拷贝数据进入 lambda，确保线程安全
        auto entry_copy = entry_ptr; // shared_ptr 捕获引用计数++
        g_threadPool.enqueue([entry_copy, query_msg, size]()
                             {
            try
            {
                //一毫秒高频压力测试
                //std::this_thread::sleep_for(std::chrono::milliseconds(50));
                entry_copy->cb(query_msg, size); // 执行回调
            }
            catch (const std::exception &e)
            {
                std::lock_guard<std::mutex> lock(callback_mutex);
                entry_copy->failure_count++;
                LOG_CC("Callback [" + entry_copy->func_name + "] exception: " + e.what());

                if (entry_copy->failure_count >= FAILURE_THRESHOLD)
                {
                    LOG_CC("Removing callback [" + entry_copy->func_name + "] after " +
                           std::to_string(entry_copy->failure_count) + " failures.");

                    g_named_callbacks.remove_if([entry_copy](const CallbackEntry &cb) {
                        return cb.func_name == entry_copy->func_name;
                    });
                }
            }
            catch (...)
            {
                std::lock_guard<std::mutex> lock(callback_mutex);
                entry_copy->failure_count++;
                LOG_CC("Callback [" + entry_copy->func_name + "] unknown exception.");

                if (entry_copy->failure_count >= FAILURE_THRESHOLD)
                {
                    LOG_CC("Removing callback [" + entry_copy->func_name + "] after " +
                           std::to_string(entry_copy->failure_count) + " failures.");

                    g_named_callbacks.remove_if([entry_copy](const CallbackEntry &cb) {
                        return cb.func_name == entry_copy->func_name;
                    });
                }
            } });
    }
    // g_threadPool.print_pool_status(); // test
}

/**
 * @brief Attempts to reconnect to the server if the connection is lost.
 */
void ipc_reconnect_to_server()
{
    // Close the old connection
    int socket_fd = socket_client.load();
    client.close_socket(socket_fd);
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait for 2 seconds before reconnecting

    // Try to reopen the socket and connect to the server
    socket_fd = client.open_socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        std::cerr << "App: Failed to open socket. Retrying...\n";
        return; // If opening the socket fails, exit the function without further operations
    }

    socket_client.store(socket_fd);

    // Try to connect to the IPC server
    int connect_result = client.connect_to_socket(socket_fd, ipc_socket_path_name);
    if (connect_result < 0)
    {
        std::cerr << "App: Failed to reconnect to the server. Retrying...\n";
        // Check if the IPC process is running, if not, start the process
        if (!is_ipc_socket_server_process_running(ipc_server_name))
        {
            std::cout << "Client: Reconnect failed due to IPC server not running,Starting the server...\n";
            start_process_as_server(ipc_server_path_name);
        }
    }
    else
    {
        std::cout << "App: Successfully reconnected to the server.\n";
        // If data pushing is required, start the request to push data
        if (request_push_data)
        {
            start_request_push_data(true);
        }
    }
}

// Function to check if data packet is complete
DataMessage check_complete_data_packet(std::vector<uint8_t> &buffer, DataMessage &query_msg)
{
    // Reset to invalid state (used for isValid() check later)
    query_msg.group = -1;
    query_msg.msg = -1;

    const size_t baseLength = query_msg.get_base_length(); // Expected minimum size (header + group + msg + length)

    // If buffer is too small to even contain the base structure, skip processing
    if (buffer.size() < baseLength)
        return query_msg;

    // Scan the first max_scan bytes to find a valid header and trim junk before it
    constexpr size_t max_scan = 20;
    bool header_found = false;
    for (size_t i = 0; i + 1 < buffer.size() && i < max_scan; ++i)
    {
        uint16_t header = (buffer[i] << 8) | buffer[i + 1];
        if (header == query_msg._HEADER_)
        {
            if (i > 0)
                buffer.erase(buffer.begin(), buffer.begin() + i); // Remove junk bytes before header
            header_found = true;
            break;
        }
    }

    // If no valid header found, erase scanned bytes to avoid buffer bloating
    if (!header_found)
    {
        size_t remove_count = std::min(buffer.size(), max_scan);
        buffer.erase(buffer.begin(), buffer.begin() + remove_count);
        return query_msg;
    }

    // Now that the header is aligned at buffer[0], ensure we can read the full base structure
    if (buffer.size() < baseLength)
        return query_msg;

    // Peek into the buffer to get the length field only (without deserializing the full message)
    uint16_t length = (static_cast<uint16_t>(buffer[4]) << 8) | buffer[5];
    size_t totalLength = baseLength + length;

    // If the buffer is still not large enough for the full message, wait for more data
    if (buffer.size() < totalLength)
        return query_msg;

    // Now we have enough bytes, safely deserialize the message
    query_msg = DataMessage::deserializeMessage(buffer);

    // Verify integrity using isValid()
    if (!query_msg.isValid())
        return query_msg;

    // Remove processed message from buffer
    buffer.erase(buffer.begin(), buffer.begin() + totalLength);

    return query_msg;
}

/**
 * @brief Continuously listens for incoming responses from the server.
 * If the connection is lost, the client attempts to reconnect automatically.
 */
void ipc_receive_response_loop()
{
    std::vector<uint8_t> buffer; // Global buffer to hold incoming data
    std::string str = "octopus.ipc.app.client";
    std::vector<uint8_t> parameters(str.begin(), str.end());

    // std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait before reconnecting
    // app_send_command(MSG_GROUP_SET, MSG_IPC_SOCKET_CONFIG_IP, parameters);
    std::cout << "[App] Start running [" << socket_running.load() << "]...\n";
    while (socket_running.load()) // Ensure atomic thread-safe check
    {
        if (socket_client.load() < 0)
        {
            std::cerr << "App: No active connection, attempting to reconnect...\n";
            ipc_reconnect_to_server();
            continue;
        }

        // Use epoll-based response method for efficient high-frequency polling
        QueryResult result = client.get_response_with_epoll(socket_client, 200); // 100ms timeout

        switch (result.status)
        {
        case QueryStatus::Success:
            break; // Continue processing below
        case QueryStatus::Timeout:
            continue; // No data available yet, continue polling
        case QueryStatus::Disconnected:
            std::cerr << "App: Connection closed by server, reconnecting...\n";
            ipc_reconnect_to_server();
            continue;
        case QueryStatus::Error:
            std::cerr << "App: Connection error (errno=" << errno << "), reconnecting...\n";
            ipc_reconnect_to_server();
            continue;
        }

        // Append received data to buffer
        buffer.insert(buffer.end(), result.data.begin(), result.data.end());
        DataMessage query_msg;

        // Extract and process complete packets
        while (buffer.size() >= query_msg.get_base_length())
        {
            query_msg = check_complete_data_packet(buffer, query_msg);

            if (query_msg.isValid())
            {
                invoke_notify_response(query_msg, query_msg.get_total_length());
            }
            else
            {
                break; // Wait for more data
            }
        }
        // Optional: reduce CPU load if desired (can be tuned or removed)
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

/**
 * @brief Signal handler for cleaning up resources on interrupt signal (e.g., Ctrl+C).
 * @param signum The received signal number.
 */
void ipc_signal_handler(int signum)
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
    // Register signal handler for SIGINT to safely shutdown the application when interrupt signal is received
    signal(SIGINT, ipc_signal_handler);

    // Redirect application logs to a file for persistence and better debugging
    ipc_redirect_log_to_file();

    // Initialize the thread pool for handling asynchronous tasks in the background
    ipc_init_threadpool();

    // Start the message bus dispatcher in a separate thread
    messageBus.start();

    if (!file_exists_and_executable(ipc_server_path_name))
    {
        return;
    }
    // Attempt to open a Unix socket (AF_UNIX) with the specified protocol (SOCK_STREAM)
    int fd = client.open_socket(AF_UNIX, SOCK_STREAM, 0);
    socket_client.store(fd);

    // Check if the socket was opened successfully
    if (fd < 0)
    {
        std::cerr << "[App] Failed to open ipc socket.\n"; // Log error if socket opening fails
        return;                                            // Return early if socket couldn't be opened
    }

    if (!is_ipc_socket_server_process_running(ipc_server_name))
    {
        std::cout << "Client: IPC server not running,Starting the server...\n";
        start_process_as_server(ipc_server_path_name);
    }

    // Attempt to establish a connection with the server via the Unix socket
    int connected_result = connected_result = client.connect_to_socket(fd, ipc_socket_path_name);
    if (connected_result < 0)
    {
        std::cerr << "[App] Failed to connect to server.\n"; // Log error if connection fails
        client.close_socket(fd);                             // Close the socket before returning
        socket_client.store(-1);                             // Store an invalid socket file descriptor to indicate failure
        // return;                                              // Return early if the connection could not be established
    }
    
    // allow to start server and connect again in ipc_receiver_thread
    if (connected_result > 0)
    {
        std::cout << "[App] Successfully connected to server.\n"; // Log success when connection is established
    }

    // Attempt to initialize epoll for event-driven communication
    client.init_epoll(fd);
    // Start a new thread to handle receiving responses asynchronously
    socket_running.store(true);                                   // Set the 'running' flag to true to indicate that the application is running
    ipc_receiver_thread = std::thread(ipc_receive_response_loop); // Start the response handling thread

    // if (!client.init_epoll(fd))
    //{
    // std::cerr << "[App] Failed to initialize epoll.\n"; // Log error if epoll initialization fails
    // client.close_socket(fd);                            // Close the socket before returning
    // socket_client.store(-1);                            // Store an invalid socket file descriptor to indicate failure
    // return;                                             // Return early if epoll initialization fails
    //}
}

/**
 * @brief Cleanup function to properly close the socket and stop the receiving thread.
 */
__attribute__((destructor)) void exit_cleanup()
{
    std::cout << "App: Cleaning up resources...\n";

    // Ensure cleanup is only performed once using atomic flag
    if (!std::atomic_exchange(&socket_running, false))
    {
        std::cout << "App: Cleanup already performed, skipping...\n";
        // return;
    }

    // Stop the message bus, ensuring that it is safely stopped before continuing.
    try
    {
        messageBus.stop();
        std::cout << "App: Message bus stopped.\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "App: Error stopping message bus: " << e.what() << std::endl;
    }

    // Safely close the socket connection
    try
    {
        client.close_socket(socket_client.load());
        std::cout << "App: Socket closed.\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "App: Error closing socket: " << e.what() << std::endl;
    }

    // Ensure receiver thread is properly joined if it is joinable
    if (ipc_receiver_thread.joinable())
    {
        try
        {
            ipc_receiver_thread.join();
            std::cout << "App: Receiver thread joined successfully.\n";
        }
        catch (const std::exception &e)
        {
            std::cerr << "App: Error joining receiver thread: " << e.what() << std::endl;
        }
    }
    else
    {
        std::cout << "App: No receiver thread to join.\n";
    }
    std::cout << "App: Cleanup complete.\n";
}

void ipc_redirect_log_to_file()
{
// 重定向 stdout 到文件
#if 1
    freopen("/tmp/octopus_ipc_client.log", "w", stdout);
    // freopen("/tmp/octopus_ipc_client.log", "w", stderr);
#else
    // 打开日志文件，按追加模式写入
    std::ofstream log_file("/tmp/octopus_ipc_client.log", std::ios::app);
    // 保存原始的 std::cout 输出缓冲区
    std::streambuf *original_buffer = std::cout.rdbuf();
    // 将 std::cout 的输出重定向到文件
    std::cout.rdbuf(log_file.rdbuf());
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void start_request_push_data(bool requested)
{
    // std::vector<uint8_t> parameters(str.begin(), str.end());
    request_push_data = requested;
    if (request_push_data)
        app_send_command(MSG_GROUP_SET, MSG_IPC_SOCKET_CONFIG_FLAG, {0, 1});
}
