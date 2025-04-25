#include "ConditionVariable.hpp"
#include "Mutex.hpp"
#include "GreenThread.hpp"
#include "Scheduler.hpp"
#include <stdexcept>
#include <iostream>

namespace GreenThreads {

void ConditionVariable::wait(std::unique_lock<Mutex>& lock) {
    std::cout << "ConditionVariable::wait - Entered" << std::endl;
    auto currentThread = Scheduler::instance().getCurrentThread();
    if (!currentThread) {
        throw std::runtime_error("wait() called outside of green thread");
    }

    {
        std::cout << "ConditionVariable::wait - Adding to waiters" << std::endl;
        std::lock_guard<std::mutex> guard(cvMutex_);
        waiters_.push(currentThread);
    }

    std::cout << "ConditionVariable::wait - Unlocking mutex" << std::endl;
    lock.unlock();

    std::cout << "ConditionVariable::wait - Yielding" << std::endl;
    Scheduler::instance().yield();

    std::cout << "ConditionVariable::wait - Reacquiring lock" << std::endl;
    lock.lock();
    std::cout << "ConditionVariable::wait - Completed" << std::endl;
}

void ConditionVariable::notify_one() {
    std::shared_ptr<GreenThread> waiter;
    
    {
        std::lock_guard<std::mutex> guard(cvMutex_);
        if (waiters_.empty()) {
            return;
        }
        
        waiter = waiters_.front();
        waiters_.pop();
    }
    
    if (waiter && !waiter->isFinished()) {
        std::cout << "ConditionVariable::notify_one - Resuming waiter" << std::endl;
        waiter->resume();
    }
}

void ConditionVariable::notify_all() {
    std::queue<std::shared_ptr<GreenThread>> waitersToResume;
    
    {
        std::lock_guard<std::mutex> guard(cvMutex_);
        std::swap(waiters_, waitersToResume);
    }
    
    while (!waitersToResume.empty()) {
        auto waiter = waitersToResume.front();
        waitersToResume.pop();
        
        if (waiter && !waiter->isFinished()) {
            std::cout << "ConditionVariable::notify_all - Resuming waiter" << std::endl;
            waiter->resume();
        }
    }
}

} // namespace GreenThreads 