#include "Scheduler.hpp"
#include "GreenThread.hpp"
#include <iostream>
#include <stdexcept>

namespace GreenThreads {

Scheduler& Scheduler::getInstance() {
    static Scheduler instance;
    return instance;
}

Scheduler::Scheduler() 
    : running_(false)
    , mainFiber_(nullptr)
    , currentThread_(nullptr) {
    workerThreads_.reserve(MAX_THREADS);
}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::spawn(std::function<void()> func) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    auto thread = std::make_shared<GreenThread>(func);
    readyQueue_.push(thread);
    queueCV_.notify_one();
}

void Scheduler::yield() {
    if (!GreenThread::current()) {
        throw std::runtime_error("yield() called outside of a green thread");
    }

    std::shared_ptr<GreenThread> nextThread;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (readyQueue_.empty()) {
            return;
        }
        
        nextThread = readyQueue_.front();
        readyQueue_.pop();
        
        if (currentThread_ && !currentThread_->isFinished()) {
            readyQueue_.push(currentThread_);
        }
    }
    
    auto prevThread = currentThread_;
    
    currentThread_ = nextThread;
    
    if (nextThread) {
        nextThread->resume();
    }
    
    currentThread_ = prevThread;
}

void Scheduler::run() {
    if (running_) {
        return;
    }

    running_ = true;
    
    if (!IsThreadAFiber()) {
        mainFiber_ = ConvertThreadToFiber(nullptr);
        if (!mainFiber_) {
            throw std::runtime_error("Failed to convert thread to fiber");
        }
    } else {
        mainFiber_ = GetCurrentFiber();
    }

    try {
        while (running_) {
            std::shared_ptr<GreenThread> threadToRun;
            
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                if (readyQueue_.empty()) {
                    break;
                }
                threadToRun = readyQueue_.front();
                readyQueue_.pop();
                currentThread_ = threadToRun;
            }

            if (threadToRun) {
                try {
                    std::cout << "Resuming thread..." << std::endl;
                    threadToRun->resume();
                    std::cout << "Returned from resume" << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Thread error: " << e.what() << std::endl;
                }

                if (!threadToRun->isFinished()) {
                    std::lock_guard<std::mutex> lock(queueMutex_);
                    readyQueue_.push(threadToRun);
                }
            }
            
            currentThread_.reset();
        }
    } catch (const std::exception& e) {
        std::cerr << "Scheduler error: " << e.what() << std::endl;
        if (mainFiber_ && IsThreadAFiber()) {
            ConvertFiberToThread();
        }
        throw;
    }

    if (mainFiber_ && IsThreadAFiber()) {
        ConvertFiberToThread();
    }
}

void Scheduler::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    queueCV_.notify_all();
}

void Scheduler::schedule() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    while (readyQueue_.empty() && running_) {
        queueCV_.wait(lock);
    }

    if (!running_) {
        return;
    }

    if (!readyQueue_.empty()) {
        currentThread_ = readyQueue_.front();
        readyQueue_.pop();
    }
}

} // namespace GreenThreads 