cmake_minimum_required(VERSION 3.10)  # 降低最低要求以匹配你的版本

project(calculator VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(src)

# 找到 src 目录下所有 C 和 C++ 文件
file(GLOB SRC_FILES src/*.c src/*.cpp)

# 创建静态库 octopus_ipc_socket_lib
add_library(octopus STATIC ${SRC_FILES})

# 创建可执行文件
add_executable(octopus_server src/octopus_ipc_server.cpp)
add_executable(octopus_client src/octopus_ipc_client.cpp)

# 连接静态库
target_link_libraries(octopus_server PRIVATE octopus pthread)
target_link_libraries(octopus_client PRIVATE octopus pthread)

# 指定安装路径
install(TARGETS octopus_client octopus_server DESTINATION bin)

# 安装 systemd 服务文件
install(FILES octopus_rpc.service DESTINATION /etc/systemd/system)
