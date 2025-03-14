/**
 * @file SerialPort.cpp
 * @brief Implementation file for the SerialPort class
 * 
 * This file implements the SerialPort class methods, handling serial 
 * port operations such as opening, configuring, reading, and writing data. 
 * It also manages a background thread for receiving data asynchronously.
 * 
 * @author Leiming Li
 * @organization Octopus
 * @date 2025-03-14
 */

 #include "octopus_serialport.hpp"

 /**
  * @brief Constructor for SerialPort class
  * @param port The serial port name (e.g., "/dev/ttyS0")
  * @param baud_rate The baud rate for communication (e.g., B115200)
  */
 SerialPort::SerialPort(const std::string& port, int baud_rate)
     : portName(port), baudRate(baud_rate), serialFd(-1), isRunning(false) {}
 
 /**
  * @brief Destructor for SerialPort class
  */
 SerialPort::~SerialPort() {
     closePort();
 }
 
 /**
  * @brief Opens the serial port and configures it
  * @return True if the port is successfully opened, false otherwise
  */
 bool SerialPort::openPort() {
     serialFd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
     if (serialFd == -1) {
         std::cerr << "Failed to open serial port: " << portName << std::endl;
         return false;
     }
 
     struct termios options;
     tcgetattr(serialFd, &options);
 
     cfsetispeed(&options, baudRate);
     cfsetospeed(&options, baudRate);
 
     options.c_cflag |= (CLOCAL | CREAD);
     options.c_cflag &= ~PARENB; // No parity
     options.c_cflag &= ~CSTOPB; // One stop bit
     options.c_cflag &= ~CSIZE;
     options.c_cflag |= CS8;     // 8-bit characters
 
     options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input mode
     options.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow control
     options.c_oflag &= ~OPOST; // Disable output processing
 
     tcsetattr(serialFd, TCSANOW, &options);
 
     isRunning = true;
     readThread = std::thread(&SerialPort::readLoop, this);
 
     std::cout << "Serial port " << portName << " opened successfully." << std::endl;
     return true;
 }
 
 /**
  * @brief Closes the serial port and stops the reading thread
  */
 void SerialPort::closePort() {
     if (isRunning) {
         isRunning = false;
         if (readThread.joinable()) {
             readThread.join();
         }
     }
     if (serialFd != -1) {
         close(serialFd);
         serialFd = -1;
     }
 }
 
 /**
  * @brief Writes data to the serial port
  * @param data The string data to be sent
  * @return True if data is successfully written, false otherwise
  */
 bool SerialPort::writeData(const std::string& data) {
     if (serialFd == -1) {
         std::cerr << "Serial port not open, cannot write data." << std::endl;
         return false;
     }
 
     int bytesWritten = write(serialFd, data.c_str(), data.size());
     return bytesWritten > 0;
 }
 
 /**
  * @brief Sets a callback function to handle received data
  * @param callback The function to be called when data is received
  */
 void SerialPort::setCallback(DataCallback callback) {
     dataCallback = callback;
 }
 
 /**
  * @brief Background thread function for reading serial data
  * 
  * This function continuously reads data from the serial port in a separate 
  * thread and invokes the registered callback function when data is received.
  */
 void SerialPort::readLoop() {
     char buffer[256];
     while (isRunning) {
         int bytesRead = read(serialFd, buffer, sizeof(buffer) - 1);
         if (bytesRead > 0) {
             buffer[bytesRead] = '\0';
             std::string receivedData(buffer);
 
             std::cout << "Received: " << receivedData << std::endl;
 
             if (dataCallback) {
                 dataCallback(receivedData);
             }
         }
         usleep(10000); // Sleep 10ms to avoid CPU overload
     }
 }
 