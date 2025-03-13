

#include <mutex>
#include "octopus_ipc_socket.hpp"

void handle_client(int client_fd);

    //std::mutex client_mutex;
Socket server;
socket_path = "/tmp/octopus/ipc_socket";

void ensure_directory_exists(const char* path) {
    std::string dir = path;
    size_t pos = dir.find_last_of('/');

    if (pos != std::string::npos) {
        std::string parent_dir = dir.substr(0, pos);
        struct stat st;

        // 检查目录是否存在
        if (stat(parent_dir.c_str(), &st) == -1) {
            if (mkdir(parent_dir.c_str(), 0777) == -1) {
                std::cerr << "Failed to create directory: " << strerror(errno) << std::endl;
            }
        }
    }
}

void remove_old_socket() {
    unlink(socket_path);  // 删除旧的 socket 文件，避免 bind 失败
}

int main()
{
    std::cout << "Server process started." << std::endl;

    ensure_directory_exists(socket_path);
    remove_old_socket();
    server.open_server_socket();
    server.bind_server_to_socket();
    server.start_listening_client();

    std::cout << "Server waiting for client connections..." << std::endl;

    while (true)
    {
        int client_fd = server.wait_and_accept();
        if (client_fd < 0)
        {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

        std::thread client_thread(handle_client, client_fd);
        client_thread.detach();
    }

    server.close_server_socket();
    return 0;
}

void handle_client(int client_fd)
{
    std::cout << "Client [" << client_fd << "] connected." << std::endl;
    std::vector<int> query_vector(3);
    std::vector<int> resp_vector(1);

    while (true)
    {
        query_vector = server.get_query(client_fd);
        if (query_vector.empty())
        {
            std::cout << "Client [" << client_fd << "] disconnected." << std::endl;
            break;
        }

        int calc_result = 0;
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
                std::cerr << "Client [" << client_fd << "]: Error - Division by zero!" << std::endl;
                continue;
            }
            calc_result = query_vector[1] / query_vector[2];
            break;
        default:
            std::cerr << "Client [" << client_fd << "]: Invalid operation." << std::endl;
            continue;
        }

        resp_vector[0] = calc_result;
        server.send_response(client_fd,resp_vector);
    }

    close(client_fd);
    std::cout << "Client [" << client_fd << "] connection closed." << std::endl;
}
