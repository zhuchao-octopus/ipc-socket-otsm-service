// octopus_logger.hpp - Header file for the Logger class
// Author: ak47
// Date: 2025-03-14
// Description: 
// This file defines a Logger class that provides a static method to log messages to the console.
// The log messages are prefixed with a specified TAG and a timestamp in the format [YYYY-MM-DD HH:MM:SS].
// This logger is useful for adding timestamped logs in various modules for debugging or tracking purposes.
// 
// Usage Example:
// Logger::log("INFO", "System started successfully");
// This will output something like:
// [INFO] [2025-03-14 12:34:56] System started successfully

#include <octopus_logger.hpp>

class Logger {
public:
    // Logs a message with a TAG and timestamp
    // @param tag: The tag associated with the log message (e.g., module name or log level)
    // @param message: The actual log message to be displayed
    static void log(const std::string& tag, const std::string& message) {
        // Get the current time
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::tm buf;
        localtime_r(&in_time_t, &buf);  // Get local time

        // Output the log with the format: [TAG] [timestamp] message
        std::cout << "[" << tag << "] "
                  << "[" << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << "] "
                  << message << std::endl;
    }
};
