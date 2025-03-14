#include <iostream>
#include "./HAL/octopus_serialport.hpp"

// 回调函数，处理接收到的数据
void onDataReceived(const std::string& data) {
    std::cout << "Callback received data: " << data << std::endl;
}

int main() {
    // 创建 SerialPort 实例，指定串口设备名称和波特率
    SerialPort serial("/dev/ttyS0", B115200);

    // 打开串口
    if (!serial.openPort()) {
        std::cerr << "Failed to open serial port!" << std::endl;
        return -1;
    }

    // 设置回调函数，处理接收到的数据
    serial.setCallback(onDataReceived);

    // 发送数据
    std::string message = "Hello, SerialPort!";
    if (serial.writeData(message)) {
        std::cout << "Sent: " << message << std::endl;
    } else {
        std::cerr << "Failed to send data!" << std::endl;
    }

    // 运行一段时间等待数据
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // 关闭串口
    serial.closePort();

    return 0;
}
