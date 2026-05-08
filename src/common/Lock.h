#ifndef LOCK_H
#define LOCK_H

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <stdexcept>

// 互斥锁
class MutexLock {
public:
    MutexLock() = default;
    ~MutexLock() = default;

    MutexLock(const MutexLock&) = delete;
    MutexLock& operator=(const MutexLock&) = delete;

    void lock() {
        mutex_.lock();
    }

    void unlock() {
        mutex_.unlock();
    }

    std::mutex* get_mutex() {
        return &mutex_;
    }

private:
    std::mutex mutex_;
};

// 读写锁
class RWMutex {
public:
    RWMutex() = default;
    ~RWMutex() = default;

    RWMutex(const RWMutex&) = delete;
    RWMutex& operator=(const RWMutex&) = delete;

    void lock_read() {
        read_mutex_.lock();
        read_count_++;
        if (read_count_ == 1) {
            write_mutex_.lock();
        }
        read_mutex_.unlock();
    }

    void unlock_read() {
        read_mutex_.lock();
        read_count_--;
        if (read_count_ == 0) {
            write_mutex_.unlock();
        }
        read_mutex_.unlock();
    }

    void lock_write() {
        write_mutex_.lock();
    }

    void unlock_write() {
        write_mutex_.unlock();
    }

private:
    std::mutex read_mutex_;
    std::mutex write_mutex_;
    std::atomic<int> read_count_{0};
};

// 自动锁（RAII）
template <typename LockType>
class AutoLock {
public:
    explicit AutoLock(LockType& lock) : lock_(lock) {
        lock_.lock();
    }

    ~AutoLock() {
        lock_.unlock();
    }

    AutoLock(const AutoLock&) = delete;
    AutoLock& operator=(const AutoLock&) = delete;

    LockType& get_lock() {
        return lock_;
    }

    const LockType& get_lock() const {
        return lock_;
    }

private:
    LockType& lock_;
};

// 条件变量
template <typename LockType>
class ConditionVariable {
public:
    ConditionVariable() = default;
    ~ConditionVariable() = default;

    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;

    void wait(AutoLock<LockType>& lock) {
        cond_.wait(*(lock.get_lock().get_mutex()));
    }

    void notify_one() {
        cond_.notify_one();
    }

    void notify_all() {
        cond_.notify_all();
    }

private:
    std::condition_variable_any cond_;
};

// 针对 MutexLock 的特化版本
using ConditionVariableMutex = ConditionVariable<MutexLock>;

#endif // LOCK_H