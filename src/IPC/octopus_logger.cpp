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
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <string>
#include <fstream>

// Logs a message with a TAG and timestamp (with microseconds precision)
void Logger::log(const std::string &tag, const std::string &message, const std::string &func_name)
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    // Default to __func__ if no func_name is provided
    std::string function_name = func_name.empty() ? __func__ : func_name;

    std::tm buf;
    localtime_r(&in_time_t, &buf); // Get local time

    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    auto milliseconds = microseconds / 1000;
    auto micros = microseconds % 1000;

    std::cout << "[" << tag << "] "
              << "[" << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << "."
              << std::setw(3) << std::setfill('0') << milliseconds % 1000 << "." // milliseconds
              << std::setw(3) << std::setfill('0') << micros << "] "
              << "[" << function_name << "] " // Function name
              << message << std::endl;
}

// Logs a message without a TAG (default TAG: "OINFOR")
void Logger::log(const std::string &message, const std::string &func_name)
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    // Default to __func__ if no func_name is provided
    std::string function_name = func_name.empty() ? __func__ : func_name;
    std::tm buf;
    localtime_r(&in_time_t, &buf); // Get local time

    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    auto milliseconds = microseconds / 1000;
    auto micros = microseconds % 1000;

    std::cout << "[OINFOR] "
              << "[" << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << "."
              << std::setw(3) << std::setfill('0') << milliseconds % 1000 << "." // milliseconds
              << std::setw(3) << std::setfill('0') << micros << "] "
              << "[" << function_name << "] " // Function name
              << message << std::endl;
}

// Logs a message with timestamp accurate to microseconds
void Logger::log_with_microseconds(const std::string &tag, const std::string &message)
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::tm buf;
    localtime_r(&in_time_t, &buf); // Get local time

    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    auto milliseconds = microseconds / 1000;
    auto micros = microseconds % 1000;

    std::cout << "[" << tag << "] "
              << "[" << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << "."
              << std::setw(3) << std::setfill('0') << milliseconds % 1000 << "." // milliseconds
              << std::setw(3) << std::setfill('0') << micros << "] "
              << message << std::endl;
}

// Logs a message with timestamp accurate to microseconds without a TAG (default TAG: "OINFOR")
void Logger::log_with_microseconds(const std::string &message)
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::tm buf;
    localtime_r(&in_time_t, &buf); // Get local time

    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    auto milliseconds = microseconds / 1000;
    auto micros = microseconds % 1000;

    std::cout << "[OINFOR] "
              << "[" << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << "."
              << std::setw(3) << std::setfill('0') << milliseconds % 1000 << "." // milliseconds
              << std::setw(3) << std::setfill('0') << micros << "] "
              << message << std::endl;
}

// Logs a message to a file with a TAG
void Logger::log_to_file(const std::string &tag, const std::string &message, const std::string &func_name)
{
    std::ofstream log_file("app.log", std::ios::app);
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    // Default to __func__ if no func_name is provided
    std::string function_name = func_name.empty() ? __func__ : func_name;
    std::tm buf;
    localtime_r(&in_time_t, &buf); // Get local time

    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    auto milliseconds = microseconds / 1000;
    auto micros = microseconds % 1000;

    log_file << "[" << tag << "] "
             << "[" << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << "."
             << std::setw(3) << std::setfill('0') << milliseconds % 1000 << "." // milliseconds
             << std::setw(3) << std::setfill('0') << micros << "] "
             << "[" << function_name << "] " // Function name
             << message << std::endl;
    log_file.close();
}

// Logs a message to a file without a TAG (default TAG: "OINFOR")
void Logger::log_to_file(const std::string &message, const std::string &func_name)
{
    std::ofstream log_file("app.log", std::ios::app);
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    // Default to __func__ if no func_name is provided
    std::string function_name = func_name.empty() ? __func__ : func_name;
    std::tm buf;
    localtime_r(&in_time_t, &buf); // Get local time

    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    auto milliseconds = microseconds / 1000;
    auto micros = microseconds % 1000;

    log_file << "[OINFOR] "
             << "[" << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << "."
             << std::setw(3) << std::setfill('0') << milliseconds % 1000 << "." // milliseconds
             << std::setw(3) << std::setfill('0') << micros << "] "
             << "[" << function_name << "] " // Function name
             << message << std::endl;
    log_file.close();
}

void Logger::rotate() {

}
