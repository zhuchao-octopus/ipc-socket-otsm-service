# 设置 CMake 系统和处理器类型
set(CMAKE_SYSTEM_NAME Linux)  # 目标系统是 Linux
set(CMAKE_SYSTEM_PROCESSOR riscv64)  # 目标处理器是 RISC-V 64 位

# 指定交叉编译工具链路径
set(TOOLCHAIN_PATH "/smb/Xuantie-900-gcc-linux-6.6.0-musl64-x86_64-V2.10.2-20240904/bin")

# 设置交叉编译器
set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/riscv64-unknown-linux-musl-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/riscv64-unknown-linux-musl-g++)

# 设置 sysroot 和 root 路径
set(CMAKE_SYSROOT "/smb/Xuantie-900-gcc-linux-6.6.0-musl64-x86_64-V2.10.2-20240904/sysroot")
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})

# 配置 CMake 查找库和头文件的方式
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)  # 不查找程序
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)  # 仅查找库文件
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)  # 仅查找头文件

# 线程库支持配置
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# 设置 C++ 编译器标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/smb/Xuantie-900-gcc-linux-6.6.0-musl64-x86_64-V2.10.2-20240904/include")

# 设置链接器标志
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/smb/Xuantie-900-gcc-linux-6.6.0-musl64-x86_64-V2.10.2-20240904/lib -lpthread")

# 设置 Make 工具路径
find_program(CMAKE_MAKE_PROGRAM NAMES make gmake PATHS /usr/bin /bin /smb/Xuantie-900-gcc-linux-6.6.0-musl64-x86_64-V2.10.2-20240904/bin)

# 如果未找到 Make 工具，抛出错误信息
if(NOT CMAKE_MAKE_PROGRAM)
    message(FATAL_ERROR "Make program not found! Install 'make' or check toolchain path.")
endif()

