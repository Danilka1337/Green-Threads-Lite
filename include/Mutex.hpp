#pragma once

#include "GreenThread.hpp"
#include <atomic>
#include <queue>
#include <memory>

namespace GreenThreads {

class ConditionVariable;

class Mutex {
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();
    bool try_lock();

private:
    friend class ConditionVariable;
    std::atomic<bool> locked_;
    std::queue<std::shared_ptr<GreenThread>> waitingThreads_;
};

} // namespace GreenThreads 