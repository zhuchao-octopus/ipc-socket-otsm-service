# 定义源文件，排除可执行文件
file(GLOB IPC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

# 创建动态库 OIPC，排除 server 和 client
list(REMOVE_ITEM IPC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/octopus_ipc_server.cpp)
list(REMOVE_ITEM IPC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/octopus_ipc_client.cpp)

# 创建动态库 OIPC
add_library(OIPC SHARED ${IPC_SOURCES})

# 创建可执行文件 octopus_ipc_server
add_executable(octopus_ipc_server ${CMAKE_CURRENT_SOURCE_DIR}/octopus_ipc_server.cpp)
# 创建可执行文件 octopus_ipc_client
add_executable(octopus_ipc_client ${CMAKE_CURRENT_SOURCE_DIR}/octopus_ipc_client.cpp)

# 将 octopus_ipc_server 和 octopus_ipc_client 与 OIPC 库链接
target_link_libraries(octopus_ipc_server PRIVATE OIPC)
target_link_libraries(octopus_ipc_client PRIVATE OIPC)
target_link_libraries(octopus_ipc_server PRIVATE dl)

# 让其他模块可以使用 OIPC 头文件
target_include_directories(OIPC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 设置 IPC 库的安装路径
install(TARGETS OIPC DESTINATION /usr/local/lib)

# 安装 IPC 头文件
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION /usr/local/include
        FILES_MATCHING PATTERN "*.hpp")

# 设置可执行文件的安装路径
install(TARGETS octopus_ipc_server octopus_ipc_client
        DESTINATION /usr/local/bin)

# 安装服务文件
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/octopus_ipc_server.service
    DESTINATION /etc/systemd/system
)


