#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <sstream>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Log {
public:
    static Log* get_instance() {
        static Log instance;
        return &instance;
    }

    bool init(const std::string& filename, LogLevel level = LogLevel::DEBUG) {
        log_file_.open(filename, std::ios::app);
        if (!log_file_.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
            return false;
        }
        log_level_ = level;
        return true;
    }

    void set_level(LogLevel level) {
        log_level_ = level;
    }

    void debug(const std::string& message) {
        log(LogLevel::DEBUG, message);
    }

    void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }

    void warn(const std::string& message) {
        log(LogLevel::WARN, message);
    }

    void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }

private:
    Log() : log_level_(LogLevel::DEBUG) {}
    ~Log() {
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }

    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    void log(LogLevel level, const std::string& message) {
        if (level < log_level_) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        
        std::time_t now = std::time(nullptr);
        char time_str[20];
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        std::string level_str;
        switch (level) {
            case LogLevel::DEBUG: level_str = "[DEBUG]"; break;
            case LogLevel::INFO:  level_str = "[INFO]";  break;
            case LogLevel::WARN:  level_str = "[WARN]";  break;
            case LogLevel::ERROR: level_str = "[ERROR]"; break;
        }

        std::string log_message = std::string(time_str) + " " + level_str + " " + message + "\n";

        std::cout << log_message;
        if (log_file_.is_open()) {
            log_file_ << log_message;
            log_file_.flush();
        }
    }

    std::ofstream log_file_;
    LogLevel log_level_;
    std::mutex mutex_;
};

// 便捷宏
#define LOG_DEBUG(msg) Log::get_instance()->debug(msg)
#define LOG_INFO(msg)  Log::get_instance()->info(msg)
#define LOG_WARN(msg)  Log::get_instance()->warn(msg)
#define LOG_ERROR(msg) Log::get_instance()->error(msg)

#endif // LOG_H