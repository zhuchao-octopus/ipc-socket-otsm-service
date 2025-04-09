
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
#include <algorithm>
#include "octopus_ipc_ptl.hpp"
#include "octopus_ipc_socket.hpp"

std::unordered_map<std::string, int> operations = {
    {"help", MSG_GROUP_0},
    {"set", MSG_GROUP_SET},
    {"subtract", 2},
    {"multiply", 3},
    {"divide", 4},
    {"car", MSG_GROUP_CAR}};

// Signal handler for clean-up on interrupt (e.g., Ctrl+C)
void signal_handler(int signum)
{
    std::cout << "Client: Interrupt signal received. Cleaning up...\n";
    exit(signum);
}

DataMessage parse_arguments(int argc, char *argv[], std::vector<std::string> &original_args)
{
    DataMessage data_msg;
    data_msg.group = 0; // Default group, can be modified or specified from parameters
    data_msg.msg = 0;
    data_msg.length = 0;
    // Save the original argument strings
    for (int i = 0; i < argc; ++i)
    {
        original_args.emplace_back(argv[i]);
    }

    // Ensure there's at least one command (excluding the program name)
    if (argc < 3)
    {
        std::cerr << "Client: Error. No operation command or message ID provided!" << std::endl;
        return data_msg;  // Return with default values
    }

    // Find the operation code (group)
    auto key_iter = operations.find(argv[1]);
    data_msg.group = (key_iter != operations.end()) ? key_iter->second : 0;

    // Ensure the second argument is a valid message ID (argv[2])
    try
    {
        // Parse the message ID from argv[2]
        data_msg.msg = static_cast<uint8_t>(std::stoul(argv[2]));  // Expecting message ID in the range of uint8_t (0-255)
    }
    catch (const std::exception &e)
    {
        std::cerr << "Client: Error. Invalid message ID '" << argv[2] << "'. Using 0 instead.\n";
        data_msg.msg = 0;  // Default to 0 if invalid
    }

    // Parse the remaining arguments as uint8_t and add them to data
    for (int i = 3; i < argc; ++i)
    {
        try
        {
            int val = std::stoi(argv[i]);
            // Check the range of the argument and clamp it to 0-255
            if (val < 0 || val > 255)
            {
                std::cerr << "Client: Warning Argument '" << argv[i] << "' out of range (0-255). Clamped to fit.\n";
            }
            uint8_t uval = static_cast<uint8_t>(std::clamp(val, 0, 255));
            data_msg.data.push_back(uval);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Client: Warning. Cannot convert '" << argv[i] << "' to integer. Using 0 instead.\n";
            data_msg.data.push_back(0);  // Add 0 if conversion fails
        }
    }

    return data_msg;
}


int main(int argc, char *argv[])
{
    Socket client;
    int socket_client = -1;
    std::vector<std::string> original_arguments;
    DataMessage data_message = parse_arguments(argc, argv, original_arguments);
    data_message.print("Client main");

    // 设置 SIGINT 信号处理
    signal(SIGINT, signal_handler);

    do
    {
        socket_client = client.open_socket();
        socket_client = client.connect_to_socket();

        if (socket_client < 0)
        {
            std::cerr << "Client: Invalid socket of connection" << std::endl;
            break; // 替代 goto
        }
        else
        {
            std::cout << "Client: Opened socket [" << socket_client << "] connected to server" << std::endl;
        }

        std::vector<uint8_t> serialized_data = data_message.serialize();
        client.send_query(serialized_data);

        std::pair<std::vector<uint8_t>, int> response_pair = client.get_response();
        std::vector<uint8_t> response = response_pair.first;
        int size = response_pair.second;

        std::cout << "Client: Received response: ";
        client.printf_vector_bytes(response, size);

    } while (false); // 只执行一次，但支持 break 退出

    client.close_socket();
    return 0;
}
