#pragma once

#include <functional>
#include <memory>
#include <atomic>
#include <cstdint>
#include <windows.h>

namespace GreenThreads {

class Scheduler;
class Mutex;
class ConditionVariable;

enum class ThreadPriority {
    LOW,
    NORMAL,
    HIGH
};

class GreenThread {
public:
    using Function = std::function<void()>;
    using ThreadId = uint64_t;

    GreenThread(Function func, ThreadPriority priority = ThreadPriority::NORMAL);
    ~GreenThread();

    void yield();
    void resume();
    void suspend();
    bool isFinished() const;
    ThreadId getId() const;
    ThreadPriority getPriority() const;
    void setPriority(ThreadPriority priority);
    void join();
    void detach();
    bool isJoinable() const;

private:
    friend class Scheduler;
    friend class Mutex;
    friend class ConditionVariable;
    
    static DWORD WINAPI ThreadStart(LPVOID param);
    
    Function func_;
    bool finished_;
    bool joinable_;
    ThreadId id_;
    ThreadPriority priority_;
    static std::atomic<ThreadId> nextId_;
    
    HANDLE threadHandle_;
    CONTEXT context_;
    LPVOID stack_;
    static constexpr size_t STACK_SIZE = 64 * 1024;
};

} // namespace GreenThreads 