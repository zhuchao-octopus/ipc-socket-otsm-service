# 添加子目录（HAL、IPC、OTSM）
add_subdirectory(HAL)
add_subdirectory(OTSM)
add_subdirectory(IPC)
add_subdirectory(APP)
# 生成主程序
add_executable(octopus_test main.c)

# 连接库
target_link_libraries(octopus_test PRIVATE OHAL OTSM)
