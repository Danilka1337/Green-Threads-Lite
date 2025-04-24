#pragma once

#include <functional>
#include <memory>
#include <atomic>
#include <windows.h>
#include <stdexcept>
#include <chrono>

namespace GreenThreads {

class Scheduler;

class GreenThread : public std::enable_shared_from_this<GreenThread> {
public:
    using ThreadFunction = std::function<void()>;

    enum class State {
        READY,
        RUNNING,
        SUSPENDED,
        FINISHED
    };

    explicit GreenThread(ThreadFunction func);
    ~GreenThread();

    void start();
    void resume();
    void yield();
    void run();

    bool isFinished() const;
    int getId() const;

    static void setMainFiber(LPVOID fiber);
    static LPVOID getMainFiber();
    static std::shared_ptr<GreenThread> current();

    State getState() const { return state_; }
    void setState(State state) { state_ = state; }
    
    LPVOID getFiber() const { return fiber_; }

private:
    static void WINAPI FiberStart(LPVOID param);

    ThreadFunction function_;
    LPVOID fiber_;
    LPVOID previousFiber_ = nullptr;
    State state_;
    int id_;

    static LPVOID mainFiber_;
    static thread_local std::weak_ptr<GreenThread> currentThread_;

    friend class Scheduler;
};

} // namespace GreenThreads 