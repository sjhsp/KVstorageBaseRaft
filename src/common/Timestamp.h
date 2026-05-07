#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <iostream>
#include <chrono>
#include <string>
#include <cstdint>

class Timestamp {
public:
    Timestamp() : microseconds_(0) {}
    
    explicit Timestamp(int64_t microseconds) : microseconds_(microseconds) {}

    static Timestamp now() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return Timestamp(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
    }

    int64_t to_microseconds() const {
        return microseconds_;
    }

    int64_t to_milliseconds() const {
        return microseconds_ / 1000;
    }

    int64_t to_seconds() const {
        return microseconds_ / 1000000;
    }

    std::string to_string() const {
        int64_t seconds = microseconds_ / 1000000;
        int64_t micros = microseconds_ % 1000000;
        
        std::time_t time = static_cast<std::time_t>(seconds);
        char time_str[20];
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
        
        char buf[30];
        snprintf(buf, sizeof(buf), "%s.%06lld", time_str, static_cast<long long>(micros));
        return std::string(buf);
    }

    bool operator<(const Timestamp& other) const {
        return microseconds_ < other.microseconds_;
    }

    bool operator<=(const Timestamp& other) const {
        return microseconds_ <= other.microseconds_;
    }

    bool operator>(const Timestamp& other) const {
        return microseconds_ > other.microseconds_;
    }

    bool operator>=(const Timestamp& other) const {
        return microseconds_ >= other.microseconds_;
    }

    bool operator==(const Timestamp& other) const {
        return microseconds_ == other.microseconds_;
    }

    bool operator!=(const Timestamp& other) const {
        return microseconds_ != other.microseconds_;
    }

    Timestamp operator+(const Timestamp& other) const {
        return Timestamp(microseconds_ + other.microseconds_);
    }

    Timestamp operator-(const Timestamp& other) const {
        return Timestamp(microseconds_ - other.microseconds_);
    }

private:
    int64_t microseconds_;
};

#endif // TIMESTAMP_H