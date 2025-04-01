# 获取 HAL 目录下所有的 .cpp 和 .c 文件
file(GLOB HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

# 创建 HAL 动态库
add_library(OHAL SHARED ${HAL_SOURCES})

# 设置 HAL 头文件路径
target_include_directories(OHAL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 设置 HAL 库的安装路径
install(TARGETS OHAL DESTINATION /usr/local/lib)

# 安装 HAL 头文件
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION /usr/local/include
        FILES_MATCHING PATTERN "*.hpp")


