
/**
 * @file client.cpp
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

#include <iomanip>
#include <map>
#include "octopus_ipc_socket.hpp"

std::unordered_map<std::string, int> operations = {
    {"help", 0},
    {"set",  1},
    {"subtract", 2},
    {"multiply", 3},
    {"divide", 4},
    {"car", 11}
};

// Signal handler for clean-up on interrupt (e.g., Ctrl+C)
void signal_handler(int signum)
{
    std::cout << "Client: Interrupt signal received. Cleaning up...\n";
    exit(signum);
}

std::vector<int> parse_arguments(int argc, char *argv[], std::vector<std::string> &original_args)
{
    std::vector<int> int_args;
    
    // 保存原始字符串参数
    for (int i = 0; i < argc; ++i)
    {
        original_args.emplace_back(argv[i]);
    }

    // 确保至少有一个参数（程序名不算）
    if (argc < 2)
    {
        std::cerr << "Error: No operation code provided!" << std::endl;
        int_args.push_back(0);  // 默认操作码 0
        return int_args;
    }

    // 查找操作码
    auto key_iterator = operations.find(argv[1]);
    int operation_code = (key_iterator != operations.end()) ? key_iterator->second : 0;
    int_args.push_back(operation_code); // 把操作码作为第一个元素

    // 解析剩余参数（从 argv[2] 开始）
    for (int i = 2; i < argc; ++i)
    {
        try
        {
            int_args.push_back(std::stoi(argv[i])); // 转换字符串为整数
        }
        catch (const std::exception &e)
        {
            std::cerr << "Warning: Cannot convert argument '" << argv[i] << "' to integer. Defaulting to 0." << std::endl;
            int_args.push_back(0); // 转换失败时填充默认值 0
        }
    }

    return int_args;
}

int main(int argc, char *argv[])
{
   
    Socket client;
    int socket_client;
    std::vector<std::string> original_arguments;
    std::vector<int> integer_arguments = parse_arguments(argc, argv, original_arguments);
    // 打印整数转换结果
    std::cout << "Client: Converted integers (starting from index 1):" << std::endl;
    for (const auto &num : integer_arguments)
    {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    // std::vector<int> response_vector;
    // Set up signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, signal_handler);
    socket_client = client.open_socket();
    socket_client = client.connect_to_socket();

    if (socket_client < 0)
    {
        std::cerr << "Client: Invalid socket of connection" << std::endl;
        goto exit_loop;
    }
    else
    {
        std::cout << "Client: Open a socket [" << socket_client << "] connected to server" << std::endl;
    }

    client.send_query(integer_arguments);

    // while (true)
    {
        // auto [response, size] = client.get_response();
        std::pair<std::vector<int>, int> response_pair = client.get_response();
        std::vector<int> response = response_pair.first;
        int size = response_pair.second;
        std::cout << "Client: Received response: ";
        client.printf_vector_bytes(response, size);
    }

exit_loop:
    client.close_socket();
    return 0;
}
