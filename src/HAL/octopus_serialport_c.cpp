/**
 * @brief Creates and opens a serial port.
 *
 * This function initializes a SerialPort C++ object and returns it as an opaque handle.
 * The object internally manages file descriptors, settings, and I/O threads.
 *
 * @param port      Path to the serial device (e.g., "/dev/ttyUSB0", "COM3").
 * @param baud_rate The baud rate to use (e.g., 9600, 115200).
 * @return A valid SerialPortHandle on success, or NULL on failure.
 */

#include "octopus_serialport_c.h"
#include "octopus_serialport.hpp"

extern "C"
{
    SerialPortHandle serialport_create(const char *port, int baud_rate)
    {
        if (!port || baud_rate <= 0)
        {
            return nullptr;
        }

        try
        {
            return new SerialPort(port, baud_rate);
        }
        catch (...)
        {
            return nullptr; // Ensure exception safety for C ABI
        }
    }

    /**
     * @brief Destroys the serial port object and releases its resources.
     *
     * @param handle The SerialPortHandle returned by serialport_create().
     */
    void serialport_destroy(SerialPortHandle handle)
    {
        if (handle)
        {
            delete static_cast<SerialPort *>(handle);
        }
    }

    /**
     * @brief Sends data over the serial port.
     *
     * This function sends a null-terminated string as a binary block.
     *
     * @param handle A valid SerialPortHandle.
     * @param data   Pointer to the data to send (must be null-terminated).
     * @return 1 if success, 0 if sending failed, -1 if input is invalid.
     */
    int serialport_write(SerialPortHandle handle, const char *data)
    {
        if (!handle || !data)
        {
            return -1;
        }

        SerialPort *serial = static_cast<SerialPort *>(handle);
        return serial->writeData(std::string(data)) ? 1 : 0;
    }

    /**
     * @brief Registers a callback to be invoked when data is received.
     *
     * The callback is called from the internal receive loop with a pointer to the received data.
     *
     * @param handle   A valid SerialPortHandle.
     * @param callback A function pointer that receives raw data and its length.
     */
    void serialport_set_callback(SerialPortHandle handle, DataCallback callback)
    {
        if (!handle || !callback)
        {
            return;
        }

        SerialPort *serial = static_cast<SerialPort *>(handle);

        // Copy the data into a lambda-safe string to ensure memory validity
        serial->setCallback([callback](const std::string &data)
                            {
                                if (!data.empty())
                                {
                                    callback(data.c_str(), static_cast<int>(data.size()));
                                } });
    }
}
