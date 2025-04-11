/**
 * @file SerialPort.cpp
 * @brief Optimized implementation file for the SerialPort class using epoll() for efficient asynchronous serial communication.
 *
 * This file contains an improved version of the SerialPort class designed to provide
 * higher performance and responsiveness, particularly suitable for high-frequency car systems, including CAN data parsing.
 */

#include "octopus_serialport.hpp"
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>
#include <thread>

// Constructor
/**
 * @brief Constructs a SerialPort object with the specified port name and baud rate.
 *
 * @param port The name of the serial port (e.g., "/dev/ttyS0").
 * @param baud_rate The baud rate for communication (e.g., 115200).
 */
SerialPort::SerialPort(const std::string &port, int baud_rate)
    : portName(port), baudRate(baud_rate), serialFd(-1), epollFd(-1), isRunning(false) {}

// Destructor
/**
 * @brief Destructor for the SerialPort class. Ensures the serial port is closed.
 */
SerialPort::~SerialPort()
{
    closePort();
}

// Open and configure the serial port
/**
 * @brief Opens the serial port and configures it with the specified baud rate.
 * This method also sets up the epoll instance for efficient event-driven communication.
 *
 * @return True if the port was successfully opened and configured, false otherwise.
 */
bool SerialPort::openPort()
{
    // Open the serial port in non-blocking mode
    serialFd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serialFd == -1)
    {
        std::cerr << "Failed to open serial port: " << portName << std::endl;
        return false;
    }

    // Configure the serial port settings
    struct termios options;
    tcgetattr(serialFd, &options);
    cfsetispeed(&options, baudRate);
    cfsetospeed(&options, baudRate);
    options.c_cflag |= (CLOCAL | CREAD); // Enable receiver and local mode
    options.c_cflag &= ~PARENB;          // Disable parity
    options.c_cflag &= ~CSTOPB;          // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;                             // 8 data bits
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Disable canonical mode and echo
    options.c_iflag &= ~(IXON | IXOFF | IXANY);         // Disable software flow control
    options.c_oflag &= ~OPOST;                          // Disable output processing

    // Apply the configuration to the serial port
    tcsetattr(serialFd, TCSANOW, &options);

    // Set up epoll for efficient event handling
    epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        std::cerr << "Failed to create epoll instance." << std::endl;
        close(serialFd);
        return false;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN; // We want to monitor input events (data available for reading)
    ev.data.fd = serialFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serialFd, &ev) == -1)
    {
        std::cerr << "Failed to add serial fd to epoll." << std::endl;
        close(serialFd);
        return false;
    }

    // Start the reading loop in a separate thread
    isRunning = true;
    readThread = std::thread(&SerialPort::readLoop, this);

    std::cout << "Serial port " << portName << " opened successfully." << std::endl;
    return true;
}

// Close the serial port and stop the thread
/**
 * @brief Closes the serial port and stops the read loop thread.
 */
void SerialPort::closePort()
{
    if (isRunning)
    {
        isRunning = false;
        if (readThread.joinable())
        {
            readThread.join(); // Join the thread to ensure proper shutdown
        }
    }
    if (serialFd != -1)
    {
        close(serialFd); // Close the serial file descriptor
        serialFd = -1;
    }
    if (epollFd != -1)
    {
        close(epollFd); // Close the epoll instance
        epollFd = -1;
    }
}

// Write data to the serial port
/**
 * @brief Writes the given data to the serial port.
 *
 * @param data The data to be written to the serial port.
 * @return True if the data was successfully written, false otherwise.
 */
bool SerialPort::writeData(const std::string &data)
{
    if (serialFd == -1)
    {
        std::cerr << "Serial port not open, cannot write data." << std::endl;
        return false;
    }
    ssize_t bytesWritten = write(serialFd, data.c_str(), data.size());
    return bytesWritten == data.size();
}

// Set the callback for received data
/**
 * @brief Sets the callback function to be called when data is received from the serial port.
 *
 * @param callback The callback function that will process the received data.
 */
void SerialPort::setCallback(DataCallback callback)
{
    dataCallback = callback;
}

// Read loop using epoll() for efficient event-driven reading
/**
 * @brief The read loop listens for data availability on the serial port using epoll.
 * When data is available, it reads and processes it using the provided callback.
 */
void SerialPort::readLoop()
{
    struct epoll_event events[1]; // Array to store the events returned by epoll_wait
    char buffer[512];             // Buffer for reading data from the serial port

    // Infinite loop to continually monitor for incoming data on the serial port
    while (isRunning)
    {
        // Wait for events with epoll_wait. The function will block until an event occurs.
        int nfds = epoll_wait(epollFd, events, 1, -1); // Wait for events
        if (nfds == -1)
        {
            // If epoll_wait returns -1, it indicates an error
            // If the error is EINTR (interrupted system call), it is typically safe to retry
            if (errno == EINTR)
            {
                continue; // Retry epoll_wait if it was interrupted
            }

            // Log other errors returned by epoll_wait
            std::cerr << "Error in epoll_wait: " << strerror(errno) << std::endl;
            break; // If it's a non-recoverable error, break the loop and stop reading
        }

        // If we have events (i.e., serialFd is ready for reading)
        if (events[0].data.fd == serialFd)
        {
            // Read the available data from the serial port into the buffer
            int bytesRead = read(serialFd, buffer, sizeof(buffer));
            if (bytesRead > 0)
            {
                // If data was successfully read, convert it into a string
                std::string receivedData(buffer, bytesRead);

                // If a callback is set, call the callback function to process the received data
                if (dataCallback)
                {
                    dataCallback(receivedData); // Process the received data
                }
            }
        }
    }

    // Optionally, you could perform any necessary cleanup or recovery actions here.
}
