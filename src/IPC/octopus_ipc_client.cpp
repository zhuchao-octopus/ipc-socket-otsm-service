
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
    {"add", 1},
    {"subtract", 2},
    {"multiply", 3},
    {"divide", 4},
    {"car", 11}};

// Signal handler for clean-up on interrupt (e.g., Ctrl+C)
void signal_handler(int signum)
{
    std::cout << "Client: Interrupt signal received. Cleaning up...\n";
    exit(signum);
}

int main(int argc, char *argv[])
{
    std::unordered_map<std::string, int>::iterator key_iterator;
    Socket client;
    int socket_client;
    int key_operation = 0;
    int key_number1 = 0;
    int key_number2 = 0;

    std::vector<int> query_vector;
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

    try
    {
        if (argc >= 2)
        {
            key_iterator = operations.find(argv[1]);
            key_operation = (key_iterator != operations.end()) ? key_iterator->second : 0;
        }
        else
        {
            key_operation = 0;  // 设置默认值
        }
        if (argc == 3)
        {
            key_number1 = std::stoi(argv[2]);
            key_number2 = 0;
        }
        else if (argc == 4)
        {
            key_number1 = std::stoi(argv[2]);
            key_number2 = std::stoi(argv[3]);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Client: Error while parsing arguments: " << e.what() << std::endl;
        goto exit_loop;
    }

    std::cout << "Client: Sending query: " << key_operation << " " << key_number1 << " " << key_number2 << std::endl;
    query_vector = {key_operation, key_number1, key_number2};
    client.send_query(query_vector);

    // while (true)
    {
        auto [response, size] = client.get_response();
        std::cout << "Client: Received response: ";
        client.printf_vector_bytes(response, size);
    }

exit_loop:
    client.close_socket();
    return 0;
}

int parser_parameter(int argc, char *argv[])
{
}
