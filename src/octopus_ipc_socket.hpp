#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cstring>
#include <string>
#include <sys/stat.h>


class Socket {

    private:

        // socket definitions
        const char* socket_path;
        int domain;
        int type;
        int protocol;

        // server definitions
        int socket_server;
        int max_waiting_requests;
        sockaddr_un addr;

        // query response definitions
        int operation;
        int number1;
        int number2;
        int result;
        ssize_t query_bytesRead;
        ssize_t resp_bytesRead;

        // client definitions
        int socket_client;

 
    public:

        // query response definitions
        int client_fd;
        int query_buffer_size;
        int resp_buffer_size;

        // Constructor
        Socket();

        // socket member functions
        void open_server_socket();
        void open_client_socket();
        void close_server_socket();
        void close_client_socket();

        // server member functions
        void bind_server_to_socket();
        void start_listening_client();
        void wait_and_accept();
        void send_response(std::vector<int> &resp_vector);
        std::vector<int> get_query();

        // client member functions
        void connect_to_server();
        void send_query(std::vector<int> &query_vector);
        std::vector<int> get_response();

};