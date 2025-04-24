#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <windows.h>

namespace GreenThreads {

class GreenThread;

class Scheduler {
public:
    static Scheduler& instance();
    
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    ~Scheduler();

    void addThread(std::shared_ptr<GreenThread> thread);
    void start();
    void stop();
    void run();
    void yield();
    
    LPVOID getSchedulerFiber() const;
    std::shared_ptr<GreenThread> getCurrentThread() const;

private:
    Scheduler();
    
    static void WINAPI SchedulerFiberStart(LPVOID param);

    std::deque<std::shared_ptr<GreenThread>> readyQueue_;
    std::set<std::shared_ptr<GreenThread>> runningThreads_;
    std::mutex queueMutex_;
    LPVOID schedulerFiber_;
    std::weak_ptr<GreenThread> currentThread_;
    bool running_;
};

} // namespace GreenThreads 