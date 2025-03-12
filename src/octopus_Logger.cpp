/*
 * Logger.cpp
 *
 *  Created on: 2025年3月12日
 *      Author: Administrator
 */

#include <Logger.h>


class Logger {
public:
    // 输出日志，带有 TAG 和时间戳
    static void log(const std::string& tag, const std::string& message) {
        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::tm buf;
        localtime_r(&in_time_t, &buf);  // 获取本地时间

        // 输出日志格式
        std::cout << "[" << tag << "] "
                  << "[" << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << "] "
                  << message << std::endl;
    }
};
