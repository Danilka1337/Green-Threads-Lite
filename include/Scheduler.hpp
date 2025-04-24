#pragma once

#include <queue>
#include <memory>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <vector>
#include <windows.h>

namespace GreenThreads {

class GreenThread;

class Scheduler {
public:
    static Scheduler& getInstance();

    void spawn(std::function<void()> func);
    void yield();
    void run();
    void stop();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

private:
    Scheduler();
    ~Scheduler();

    void schedule();
    void threadFunction();

    std::queue<std::shared_ptr<GreenThread>> readyQueue_;
    std::shared_ptr<GreenThread> currentThread_;
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    std::atomic<bool> running_{false};
    std::vector<std::thread> workerThreads_;
    LPVOID mainFiber_{nullptr};
    static constexpr size_t MAX_THREADS = 4;
};

} // namespace GreenThreads 