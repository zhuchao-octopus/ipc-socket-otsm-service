# 定义源文件，排除可执行文件
file(GLOB APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

# 创建动态库 OUI
add_library(OUI SHARED ${APP_SOURCES})


# 让其他模块可以使用 OUI 头文件
target_include_directories(OUI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 设置 OUI 库的安装路径
install(TARGETS OUI DESTINATION /usr/local/lib)

# 安装 IPC 头文件
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION /usr/local/include
        FILES_MATCHING PATTERN "*.hpp")

