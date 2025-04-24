#pragma once

#include "GreenThread.hpp"
#include <queue>
#include <memory>

namespace GreenThreads {

class Mutex;

class ConditionVariable {
public:
    ConditionVariable();
    ~ConditionVariable();

    void wait(Mutex& mutex);
    void notify_one();
    void notify_all();

private:
    std::queue<std::shared_ptr<GreenThread>> waitingThreads_;
};

} // namespace GreenThreads 