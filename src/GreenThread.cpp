#include "GreenThread.hpp"
#include <stdexcept>
#include <iostream>
#include "Scheduler.hpp"

namespace GreenThreads {

thread_local GreenThread* GreenThread::currentThread_ = nullptr;
thread_local LPVOID GreenThread::currentFiber_ = nullptr;

GreenThread::GreenThread(ThreadFunction func) 
    : func_(std::move(func))
    , fiber_(nullptr)
    , finished_(false) {
    fiber_ = CreateFiber(0, &GreenThread::FiberStart, this);
    if (!fiber_) {
        throw std::runtime_error("Failed to create fiber");
    }
}

GreenThread::~GreenThread() {
    if (fiber_) {
        DeleteFiber(fiber_);
        fiber_ = nullptr;
    }
}

void WINAPI GreenThread::FiberStart(LPVOID param) {
    auto thread = static_cast<GreenThread*>(param);
    currentThread_ = thread;
    
    try {
        thread->func_();
    } catch (const std::exception& e) {
        std::cerr << "Exception in green thread: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in green thread" << std::endl;
    }
    
    thread->finished_ = true;
    std::cout << "Thread finished, switching back to scheduler" << std::endl;
    
    currentThread_ = nullptr;
    
    if (currentFiber_) {
        SwitchToFiber(currentFiber_);
    } else {
        std::cerr << "Error: No scheduler fiber to return to!" << std::endl;
    }
}

void GreenThread::resume() {
    if (finished_ || !fiber_) {
        return;
    }

    LPVOID schedulerFiber = GetCurrentFiber();
    if (schedulerFiber == nullptr || schedulerFiber == (LPVOID)0x1E00) {
        std::cerr << "Warning: Current fiber is invalid, cannot resume" << std::endl;
        return;
    }
    
    currentFiber_ = schedulerFiber;
    
    GreenThread* prevThread = currentThread_;
    
    currentThread_ = this;
    
    std::cout << "Switching to fiber..." << std::endl;
    
    SwitchToFiber(fiber_);
    
    std::cout << "Returned from fiber switch" << std::endl;
    
    currentThread_ = prevThread;
}

bool GreenThread::isFinished() const {
    return finished_;
}

GreenThread* GreenThread::current() {
    return currentThread_;
}

void GreenThread::run() {
    try {
        func_();
    } catch (const std::exception& e) {
        std::cerr << "Exception in green thread: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in green thread" << std::endl;
    }
    finished_ = true;
}

} // namespace GreenThreads 