#include "octopus_ipc_socket.hpp"

static void signal_handler(int signum)
{
    std::cout << " !! Received signal: " << signum << std::endl;
    /// std::exit(-1);
}

// Constructor
Socket::Socket()
{
    // create domain socket
    socket_path = "/tmp/octopus/ipc_socket";
    domain = AF_UNIX;
    type = SOCK_STREAM;
    protocol = 0;

    // create signals to signal handler function
    signal(SIGINT, signal_handler);
    signal(SIGKILL, signal_handler);
    signal(SIGTERM, signal_handler);

    // server address for binding to socket
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    // server parameters
    int max_waiting_requests = 5;
}

int Socket::open_socket()
{
    socket_fd = socket(domain, type, protocol);

    if (socket_fd == -1)
    {
        std::cerr << "socket could not be created." << std::endl;
        close_socket();
        /// std::exit(-1);
    }

    return socket_fd;
}

int Socket::close_socket()
{
    close(socket_fd);
    if (socket_fd >= 0)
        std::cout << "close socket [" << socket_fd << "]" << std::endl;
    return socket_fd;
}

void Socket::bind_server_to_socket()
{
    if (bind(socket_fd, (sockaddr *)&addr, sizeof(addr)) == -1)
    {
        std::cerr << "Server Socket could not be bound to socket." << std::endl;
        close_socket();
        /// std::exit(-1);
    }

    std::cout << "Server socket opened with [" << socket_fd << "] " << addr.sun_family << " domain" << std::endl;
    std::cout << "Server socket opened with [" << socket_fd << "] " << addr.sun_path << " path" << std::endl;

    int permission = chmod(socket_path, S_IRWXU | S_IRWXG | S_IRWXO);
    if (permission == -1)
    {
        std::cerr << "Server Socket file path could not give chmod permission to client." << std::endl;
        close_socket();
        /// std::exit(-1);
    }
}

void Socket::start_listening()
{
    std::cout << "Server started to listening." << std::endl;

    int listen_result = listen(socket_fd, max_waiting_requests);

    if (listen_result == -1)
    {
        std::cerr << "Server Listen failed." << std::endl;
        close_socket();
        /// std::exit(-1);
    }
}

int Socket::wait_and_accept()
{
    int client_fd = accept(socket_fd, NULL, NULL);

    if (client_fd == -1)
    {
        std::cerr << "Server: Client connection could not be accepted." << std::endl;
        /// close_server_socket();
        /// std::exit(-1);
    }

    std::cout << "Server accepted client connection [" << client_fd << "]" << std::endl;
    return client_fd;
}

std::vector<int> Socket::get_query(int client_fd)
{
    char buffer[query_buffer_size];
    query_bytesRead = read(client_fd, buffer, sizeof(buffer));

    if (query_bytesRead <= 0)
    {
        /// std::cout << "Server Error reading from socket. bytesRead = " << query_bytesRead << " failed." << std::endl;
        ///  close_socket();
        ///  std::exit(-1);
        return {};
    }

    std::vector<int> query_vec(query_buffer_size);
    for (int i = 0; i < query_buffer_size; i++)
    {
        query_vec[i] = buffer[i];
        /// std::cout << "Server: Received query[" << i << "]: " << static_cast<int>(buffer[i]) << std::endl;
    }

    return query_vec;
}

int Socket::send_response(int client_fd, std::vector<int> &resp_vector)
{
    char resp_buffer[resp_vector.size()];
    for (int i = 0; i < resp_vector.size(); i++)
    {
        resp_buffer[i] = resp_vector[i];
    }
    auto write_result = write(client_fd, resp_buffer, sizeof(resp_buffer));

    if (write_result == -1)
    {
        std::cerr << "Server Could not write response to client." << std::endl;
        close_socket();
        /// std::exit(-1);
    }

    /// for (int i = 0; i < resp_vector.size(); i++)
    ///{
    ///     std::cout << "Server Sent response[" << i << "]: " << static_cast<int>(resp_buffer[i]) << std::endl;
    /// }

    return 0;
}

int Socket::connect_to_socket()
{
    if (connect(socket_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        std::cerr << "Client: could not connect to server socket [" << socket_fd << "]" << std::endl;
        /// std::exit(-1);
        return -1;
    }

    ///std::cout << "Client: connected to server " << std::endl;
    return socket_fd;
}

void Socket::send_query(std::vector<int> &query_vector)
{
    char query_buffer[query_vector.size()];
    for (int i = 0; i < query_vector.size(); i++)
    {
        query_buffer[i] = query_vector[i];
    }
    auto write_result = write(socket_fd, query_buffer, sizeof(query_buffer));

    if (write_result == -1)
    {
        std::cerr << "Client: Could not write queryto socket." << std::endl;
        close_socket();
        /// std::exit(-1);
    }
}

std::vector<int> Socket::get_response()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    char result_buffer[resp_buffer_size];

    resp_bytesRead = read(socket_fd, &result_buffer, sizeof(result_buffer));

    if (resp_bytesRead == -1)
    {
        std::cerr << "Client: Could not read response from server." << std::endl;
        close_socket();
        /// std::exit(-1);
    }

    std::vector<int> result_vec(resp_buffer_size);
    for (int i = 0; i < resp_buffer_size; i++)
    {
        result_vec[i] = result_buffer[i];
    }

    return result_vec;
}
