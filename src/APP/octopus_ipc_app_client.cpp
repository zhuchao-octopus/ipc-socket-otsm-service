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
void redirect_log_to_file();

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global variables
std::atomic<bool> running{true};    // Controls the receiving loop
std::atomic<int> socket_client{-1}; // Stores socket descriptor
std::thread receiver_thread;        // Thread handling incoming responses
std::mutex callback_mutex;          // Mutex for callback synchronization
Socket client;                      // IPC socket client instance
// Initialize the global thread pool object
ThreadPool g_thread_pool(4); // 4 threads in the pool

struct CallbackEntry
{
    std::string func_name;
    OctopusAppResponseCallback cb;
};

std::list<CallbackEntry> g_named_callbacks;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

bool is_ipc_process_running(const std::string &process_name)
{
    FILE *fp;
    const size_t buffer_size = 1024;
    char buffer[buffer_size]; // 分块读取
    bool found = false;

    // 执行 ps 命令获取所有运行中的进程
    fp = popen("ps aux", "r");
    if (fp == nullptr)
    {
        /// std::cerr << "Failed to run ps command" << std::endl;
        return false;
    }

    // 分块读取 ps 命令输出的结果
    while (fread(buffer, 1, buffer_size, fp) > 0)
    {
        // 查找每个块中是否包含指定进程路径
        if (strstr(buffer, process_name.c_str()) != nullptr)
        {
            found = true;
            break;
        }
    }

    fclose(fp);
    return found;
}
void start_process(const std::string &process_path)
{
    // 使用 system 调用启动进程
    std::string command = process_path + " >> " + log_file + " 2>&1 &";
    // std::string command = process_path + " &"; // 在后台启动
    int result = system(command.c_str());
    if (result == -1)
    {
        std::cerr << "Client: Failed to start process: " << process_path << std::endl;
    }
    else
    {
        std::cout << "Client: Process started: " << process_path << std::endl;
    }
}

void app_init_threadpool()
{
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
    client.send_query(serialized_data);
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
    query_msg.length = query_msg.data.size();

    std::vector<uint8_t> serialized_data = query_msg.serializeMessage();
    client.send_query(serialized_data);
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
        LOG_CC("Registered callback: " + func_name);
    }
}

void unregister_ipc_socket_callback(OctopusAppResponseCallback callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex); // 锁定，确保线程安全
    // 使用 std::remove_if 和 lambda 表达式根据回调函数指针删除
    auto it = std::remove_if(g_named_callbacks.begin(), g_named_callbacks.end(),
                             [callback](const CallbackEntry &entry)
                             {
                                 return entry.cb == callback; // 比较回调函数指针
                             });

    if (it != g_named_callbacks.end())
    {
        LOG_CC("Unregistered callback with function pointer"); // 输出日志
        g_named_callbacks.erase(it, g_named_callbacks.end());  // 删除匹配的回调
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
    std::lock_guard<std::mutex> lock(callback_mutex);
    for (const auto &cb_entry : g_named_callbacks)
    {
        {
            g_thread_pool.enqueue([cb_entry, query_msg, size]()
                                  {
                try
                {
                    cb_entry.cb(query_msg, size);  // 执行回调
                }
                catch (const std::exception &e)
                {
                    LOG_CC("Exception during callback execution: " + std::string(e.what()));
                } });
        }
    }
    g_thread_pool.print_pool_status(); /// 测试
}

/**
 * @brief Attempts to reconnect to the server if the connection is lost.
 */
void reconnect_to_server()
{
    client.close_socket(socket_client.load());
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait before reconnecting
    // return;
    socket_client.store(client.open_socket(AF_UNIX, SOCK_STREAM, 0));
    socket_client.store(client.connect_to_socket("/tmp/octopus/ipc_socket"));

    if (socket_client.load() < 0)
    {
        std::cerr << "App: Failed to reconnect to the server. Retrying...\n";
        if (!is_ipc_process_running("octopus_ipc_server"))
        {
            start_process("/res/bin/octopus_ipc_server");
        }
    }
    else
    {
        std::cout << "App: Successfully reconnected to the server.\n";
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
void receive_response_loop()
{
    std::vector<uint8_t> buffer; // Global buffer to hold incoming data
    std::string str = "octopus.ipc.app.client";
    std::vector<uint8_t> parameters(str.begin(), str.end());

    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait before reconnecting
    app_send_command(MSG_GROUP_SET, MSG_IPC_SOCKET_CONFIG_IP, parameters);
    // app_send_command(11, 100, {});

    while (running.load()) // Ensure atomic thread-safe check
    {
        if (socket_client.load() < 0)
        {
            std::cerr << "App: No active connection, attempting to reconnect...\n";
            reconnect_to_server();
            continue;
        }

        // Get response data
        auto response_pair = client.get_response();
        std::vector<uint8_t> response = response_pair.first;
        int size = response_pair.second;

        if (size <= 0)
        {
            if (size == 0)
            {
                std::cerr << "App: Connection closed by server, reconnecting...\n";
                reconnect_to_server();
            }
            else if (size < 0)
            {
                std::cerr << "App: Connection error (errno=" << errno << "), reconnecting...\n";
                reconnect_to_server();
            }
            continue;
        }

        // Add the new response data to the global buffer
        buffer.insert(buffer.end(), response.begin(), response.end());
        DataMessage query_msg;
        // Process the buffer for complete data packets
        while (buffer.size() >= query_msg.get_base_length()) // Ensure enough data for header + group + msg + length
        {
            query_msg = check_complete_data_packet(buffer, query_msg);

            // If we have a valid DataMessage
            if (query_msg.isValid())
            {
                invoke_notify_response(query_msg, query_msg.get_total_length()); // Process the complete message
            }
            else
            {
                // If no complete packet, break and wait for more data
                break;
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
    redirect_log_to_file();
    app_init_threadpool();

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

void redirect_log_to_file()
{
// 重定向 stdout 到文件
#if 1
    // freopen("/tmp/octopus_ipc_client.log", "w", stdout);
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

void start_request_push_data()
{
    // std::vector<uint8_t> parameters(str.begin(), str.end());
    app_send_command(MSG_GROUP_SET, MSG_IPC_SOCKET_CONFIG_FLAG, {0, 1});
}
