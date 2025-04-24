#pragma once

#include <functional>
#include <memory>
#include <windows.h>
#include <stdexcept>

namespace GreenThreads {

class GreenThread {
public:
    using ThreadFunction = std::function<void()>;

    explicit GreenThread(ThreadFunction func);
    ~GreenThread();

    GreenThread(const GreenThread&) = delete;
    GreenThread& operator=(const GreenThread&) = delete;
    GreenThread(GreenThread&&) = delete;
    GreenThread& operator=(GreenThread&&) = delete;

    void resume();

    bool isFinished() const;

    static GreenThread* current();

private:
    static void WINAPI FiberStart(LPVOID param);
    void run();

    ThreadFunction func_;
    LPVOID fiber_;
    bool finished_{false};
    static thread_local GreenThread* currentThread_;
    static thread_local LPVOID currentFiber_;

    friend class Scheduler;
};

} // namespace GreenThreads 