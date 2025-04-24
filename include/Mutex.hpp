#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include "GreenThread.hpp"
#include "Scheduler.hpp"

namespace GreenThreads {

class Mutex {
public:
    Mutex() : locked_(false), owner_(nullptr) {}
    ~Mutex() = default;

    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    void lock() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (locked_) {
            auto current = Scheduler::instance().getCurrentThread();
            if (current) {
                waitQueue_.push(current);
                lock.unlock();
                current->yield();
                lock.lock();
            }
        }
        locked_ = true;
        owner_ = Scheduler::instance().getCurrentThread().get();
    }

    bool try_lock() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!locked_) {
            locked_ = true;
            owner_ = Scheduler::instance().getCurrentThread().get();
            return true;
        }
        return false;
    }

    void unlock() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!locked_) {
            throw std::runtime_error("Mutex not locked");
        }
        locked_ = false;
        owner_ = nullptr;
        if (!waitQueue_.empty()) {
            auto next = waitQueue_.front();
            waitQueue_.pop();
            next->resume();
        }
    }

    friend class std::unique_lock<Mutex>;
    friend class std::lock_guard<Mutex>;

private:
    bool locked_;
    std::mutex mutex_;
    std::queue<std::shared_ptr<GreenThread>> waitQueue_;
    GreenThread* owner_;
};

} // namespace GreenThreads 