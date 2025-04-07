#ifndef OCTOPUS_LOGGER_HPP
#define OCTOPUS_LOGGER_HPP

#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>
#include <fstream>

class Logger {
public:
    // Logs a message with a TAG, timestamp (with microseconds precision), and function name
    // @param tag: The tag associated with the log message (e.g., module name or log level)
    // @param message: The actual log message to be displayed
    // @param func_name: The name of the function from which the log is called
    static void log(const std::string& tag, const std::string& message, const std::string& func_name);

    // Logs a message without a tag (default TAG: "OINFOR")
    // @param message: The actual log message to be displayed
    // @param func_name: The name of the function from which the log is called
    static void log(const std::string& message, const std::string& func_name);

    // Logs a message with timestamp accurate to microseconds
    // @param tag: The tag associated with the log message
    // @param message: The actual log message to be displayed
    static void log_with_microseconds(const std::string& tag, const std::string& message);

    // Logs a message with timestamp accurate to microseconds without a tag (default TAG: "OINFOR")
    // @param message: The actual log message to be displayed
    static void log_with_microseconds(const std::string& message);

    // Logs a message to a file with a TAG
    // @param tag: The tag associated with the log message
    // @param message: The actual log message to be displayed
    // @param func_name: The name of the function from which the log is called
    static void log_to_file(const std::string& tag, const std::string& message, const std::string& func_name);

    // Logs a message to a file without a TAG (default TAG: "OINFOR")
    // @param message: The actual log message to be displayed
    // @param func_name: The name of the function from which the log is called
    static void log_to_file(const std::string& message, const std::string& func_name);
};

// Macro for logging with automatic function name
#define LOG_CC(...) Logger::log(__VA_ARGS__, __FUNCTION__)
#define LOG_CCC(tag, ...) Logger::log(tag, __VA_ARGS__, __FUNCTION__)
#endif // OCTOPUS_LOGGER_HPP
