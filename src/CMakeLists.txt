# 添加子目录（HAL、IPC、OTSM）
add_subdirectory(HAL)
add_subdirectory(OTSM)
add_subdirectory(IPC)

# 生成主程序
add_executable(octopus-main main.cpp)

# 连接库
target_link_libraries(octopus-main PRIVATE OHAL OIPC pthread)
