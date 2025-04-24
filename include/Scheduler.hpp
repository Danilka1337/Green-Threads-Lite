#pragma once

#include "GreenThread.hpp"
#include <memory>
#include <queue>
#include <mutex>
#include <chrono>
#include <atomic>

namespace GreenThreads {

class Scheduler {
public:
    static Scheduler& getInstance();

    void spawn(GreenThread::Function func, ThreadPriority priority = ThreadPriority::NORMAL);
    void run();
    void yield();
    void schedule();
    void sleep_for(std::chrono::milliseconds duration);
    void sleep_until(std::chrono::steady_clock::time_point time);
    std::shared_ptr<GreenThread> getCurrentThread() const;
    size_t getThreadCount() const;

private:
    Scheduler() = default;
    ~Scheduler() = default;
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    struct ThreadEntry {
        std::shared_ptr<GreenThread> thread;
        std::chrono::steady_clock::time_point wakeupTime;
        bool operator<(const ThreadEntry& other) const {
            return wakeupTime > other.wakeupTime;
        }
    };

    std::priority_queue<ThreadEntry> readyQueue_;
    std::shared_ptr<GreenThread> currentThread_;
    std::mutex schedulerMutex_;
    std::atomic<size_t> threadCount_{0};
};

} // namespace GreenThreads 