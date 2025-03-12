cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)

project(
    calculator
    VERSION 1.0
    LANGUAGES C CXX
)

include_directories(src)
add_library(octopus_ipc_socket_lib STATIC src/octopus_ipc_socket.cpp)

add_executable(octopus_server src/octopus_server.cpp)
add_executable(octopus_client src/octopus_client.cpp)

target_link_libraries(octopus_server octopus_ipc_socket_lib)
target_link_libraries(octopus_client octopus_ipc_socket_lib)

install(
    TARGETS octopus_client octopus_server
    DESTINATION /usr/local/bin
)

install(
    FILES octopus.service
    DESTINATION /etc/systemd/system
)