
#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <csignal>
#include <dlfcn.h>

typedef void (*ResponseCallback)(const std::vector<int> &response, int size);
typedef void (*RegisterCallbackFunc)(ResponseCallback callback);
static void *handle = nullptr; // 保持全局作用域

void printf_vector_bytes(const std::vector<int> &vec, int length)
{
    // std::cout << "printf_vector_bytes " << length << std::endl; //
    //  确保 length 不超过 vector 的大小
    int print_length = std::min(length, static_cast<int>(vec.size()));

    for (int i = 0; i < print_length; ++i)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')  << "0x" << (vec[i] & 0xFF) << " "; // 打印每个字节
    }
    std::cout << std::dec << std::endl; // 恢复十进制格式
}

void ipc_socket_reesponse_callback(const std::vector<int> &response, int size)
{
    printf_vector_bytes(response, size);
}

void initialize_app_client()
{
    // 加载共享库
    std::cout << "App initialize oui." << std::endl;
    handle = dlopen("libOAPPC.so", RTLD_LAZY);
    // handle = dlopen("libOTSM.so", RTLD_LAZY);
#if 1
    if (!handle)
    {
        std::cerr << "App Failed to load app client library: " << dlerror() << std::endl;
        return;
    }

    RegisterCallbackFunc register_ipc_socket_callback = (RegisterCallbackFunc)dlsym(handle, "register_ipc_socket_callback");
    if (!register_ipc_socket_callback)
    {
        std::cerr << "App Failed to load register_ipc_socket_callback function: " << dlerror() << std::endl;
        dlclose(handle);
        return;
    }

    if (register_ipc_socket_callback)
    {
        register_ipc_socket_callback(ipc_socket_reesponse_callback);
    }
#endif
}
void cleanup()
{
    if (handle)
    {
        dlclose(handle);
        std::cout << "App cleaned up oui library." << std::endl;
    }
}

int main(int argc, char *argv[])
{

    initialize_app_client();

    // 注册信号处理，确保程序退出时释放 SO
    std::signal(SIGINT, [](int)
                {
        cleanup();
        exit(0); });
    while (1)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
