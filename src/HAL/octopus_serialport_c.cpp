#include "octopus_serialport_c.h"
#include "octopus_serialport.hpp"

extern "C"
{

    // 创建并打开串口
    SerialPortHandle serialport_create(const char *port, int baud_rate)
    {
        return new SerialPort(port, baud_rate);
    }

    // 销毁串口对象
    void serialport_destroy(SerialPortHandle handle)
    {
        delete static_cast<SerialPort *>(handle);
    }

    // 发送数据
    int serialport_write(SerialPortHandle handle, const char *data)
    {
        SerialPort *serial = static_cast<SerialPort *>(handle);
        return serial->writeData(std::string(data)) ? 1 : 0;
    }

    // 设置回调
    void serialport_set_callback(SerialPortHandle handle, DataCallback callback)
    {
        SerialPort *serial = static_cast<SerialPort *>(handle);
        if (serial)
        {
            // 使用值传递避免引用被销毁
            serial->setCallback([callback](std::string data) // 传值，保证数据在callback执行期间有效
                                { callback(data.c_str(), data.size()); });
        }
    }
}
