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

    query_msg.print("Send query");

    std::vector<uint8_t> serialized_data = query_msg.serialize();
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

    std::vector<uint8_t> serialized_data = query_msg.serialize();
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
    client.close_socket();
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait before reconnecting

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
DataMessage check_complete_data_packet(std::vector<uint8_t> &buffer)
{
    DataMessage query_msg;
    query_msg.group = -1;
    query_msg.msg = -1;
    // Ensure there's enough data to process
    if (buffer.size() < sizeof(uint16_t) + sizeof(uint8_t) * 3) // header + group + msg + length
    {
        return query_msg; // Return an empty DataMessage if not enough data
    }
    // Loop to find a valid header position in the buffer
    while (buffer.size() >= 2)
    {
        uint16_t header = (buffer[0] << 8) | buffer[1];
        if (header == query_msg.HEADER)
        {
            break; // Valid header found
        }
        else
        {
            buffer.erase(buffer.begin()); // Remove the first byte and try again
        }
    }

    if (buffer.size() < sizeof(uint16_t) + sizeof(uint8_t) * 3) // header + group + msg + length
    {
        return query_msg; // Return an empty DataMessage if not enough data
    }
    // Parse the header, group, and msg
    query_msg.header = (buffer[0] << 8) | buffer[1]; // Assuming header is 2 bytes
    query_msg.group = buffer[2];
    query_msg.msg = buffer[3];

    // Read the length of the data (assuming length is 1 byte)
    query_msg.length = buffer[4];

    // total package length
    size_t packet_size = sizeof(uint16_t) + sizeof(uint8_t) * 3 + query_msg.length;
    // Check if the buffer has enough data to form the full packet
    if (buffer.size() < packet_size)
    {
        return query_msg; // Not enough data yet, return incomplete DataMessage
    }

    // Now extract the data from the buffer
    query_msg.data.assign(buffer.begin() + 5, buffer.begin() + 5 + query_msg.length);
    // Remove the processed data from the buffer
    buffer.erase(buffer.begin(), buffer.begin() + packet_size); // Remove processed packet
    return query_msg;                                           // Return the complete DataMessage
}

/**
 * @brief Continuously listens for incoming responses from the server.
 * If the connection is lost, the client attempts to reconnect automatically.
 */
void receive_response_loop()
{
    std::string str = "octopus.ipc.app.client";
    std::vector<uint8_t> parameters(str.begin(), str.end());
    app_send_command(MSG_GROUP_SET, MSG_IPC_SOCKET_CONFIG_IP, parameters);

    std::vector<uint8_t> buffer; // Global buffer to hold incoming data

    while (running.load()) // Ensure atomic thread-safe check
    {
        if (socket_client.load() < 0)
        {
            std::cerr << "App: No active connection. Attempting to reconnect...\n";
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
                std::cerr << "App: Connection closed by server. Reconnecting...\n";
                reconnect_to_server();
            }
            else if (size < 0)
            {
                std::cerr << "App: Connection error (errno=" << errno << "). Reconnecting...\n";
                reconnect_to_server();
            }
            continue;
        }

        // Add the new response data to the global buffer
        buffer.insert(buffer.end(), response.begin(), response.end());

        // Process the buffer for complete data packets
        while (buffer.size() >= sizeof(uint16_t) + sizeof(uint8_t) * 3) // Ensure enough data for header + group + msg + length
        {
            DataMessage query_msg = check_complete_data_packet(buffer);

            // If we have a valid DataMessage
            if (query_msg.is_valid_packet())
            {
                invoke_notify_response(query_msg, query_msg.get_total_packet_size()); // Process the complete message
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
    // 重定向 stdout 到文件
    freopen("/tmp/octopus_ipc_client.log", "w", stdout);
    freopen("/tmp/octopus_ipc_client.log", "w", stderr);
    //redirect_log_to_file();
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
    // 打开日志文件，按追加模式写入
    std::ofstream log_file("/tmp/octopus_ipc_client.log", std::ios::app);
    // 保存原始的 std::cout 输出缓冲区
    std::streambuf *original_buffer = std::cout.rdbuf();
    // 将 std::cout 的输出重定向到文件
    std::cout.rdbuf(log_file.rdbuf());

    // 现在所有通过 std::cout 输出的内容都会被写入文件
    // std::cout << "This log is written to the file." << std::endl;
    // 完成后恢复 std::cout 的输出到原始缓冲区
    // std::cout.rdbuf(original_buffer);
    // 可以选择不恢复原始缓冲区，若不恢复，将会持续重定向输出
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void start_request_push_data()
{
    // std::vector<uint8_t> parameters(str.begin(), str.end());
    app_send_command(MSG_GROUP_SET, MSG_IPC_SOCKET_CONFIG_FLAG, {0, 1});
}
