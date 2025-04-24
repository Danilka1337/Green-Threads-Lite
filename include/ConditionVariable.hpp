#pragma once

#include <mutex>
#include <queue>
#include <memory>
#include <chrono>
#include <windows.h>
#include "GreenThread.hpp"
#include "Scheduler.hpp"

namespace GreenThreads {

class Mutex;

class ConditionVariable {
public:
    ConditionVariable() = default;
    ~ConditionVariable() = default;

    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;

    void wait(std::unique_lock<Mutex>& lock);

    template<typename Rep, typename Period>
    bool wait_for(std::unique_lock<Mutex>& lock, 
                  const std::chrono::duration<Rep, Period>& timeout) {
        auto start = std::chrono::steady_clock::now();
        auto end = start + timeout;

        auto currentThread = Scheduler::instance().getCurrentThread();
        if (!currentThread) {
            throw std::runtime_error("wait_for called outside of green thread context");
        }

        {
            std::lock_guard<std::mutex> guard(cvMutex_);
            waiters_.push(currentThread);
        }

        lock.unlock();

        Scheduler::instance().yield();

        lock.lock();

        auto now = std::chrono::steady_clock::now();
        return now < end;
    }

    template<typename Clock, typename Duration>
    bool wait_until(std::unique_lock<Mutex>& lock,
                    const std::chrono::time_point<Clock, Duration>& timeout_time) {
        return wait_for(lock, timeout_time - Clock::now());
    }

    void notify_one();
    void notify_all();

private:
    std::mutex cvMutex_;
    std::queue<std::shared_ptr<GreenThread>> waiters_;
};

} // namespace GreenThreads 