#ifndef OCTOPUS_SERIALPORT_C_H
#define OCTOPUS_SERIALPORT_C_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* SerialPortHandle;



// 创建并打开串口
SerialPortHandle serialport_create(const char* port, int baud_rate);
void serialport_destroy(SerialPortHandle handle);

// 发送数据
int serialport_write(SerialPortHandle handle, const char* data);

// 设置回调函数
typedef void (*DataCallback)(const char* data, int length);
void serialport_set_callback(SerialPortHandle handle, DataCallback callback);

#ifdef __cplusplus
}
#endif

#endif // OCTOPUS_SERIALPORT_C_H
