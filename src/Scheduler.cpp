#include "Scheduler.hpp"
#include "GreenThread.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace GreenThreads {

Scheduler& Scheduler::instance() {
    static Scheduler instance;
    return instance;
}

Scheduler::Scheduler() : schedulerFiber_(nullptr), running_(false) {}

Scheduler::~Scheduler() {
    stop();
    if (schedulerFiber_) {
        DeleteFiber(schedulerFiber_);
        schedulerFiber_ = nullptr;
    }
}

std::shared_ptr<GreenThread> Scheduler::getCurrentThread() const {
    return currentThread_.lock();
}

void Scheduler::addThread(std::shared_ptr<GreenThread> thread) {
    if (!thread) return;
    
    std::lock_guard<std::mutex> lock(queueMutex_);
    readyQueue_.push_back(std::move(thread));
}

void Scheduler::start() {
    if (running_) return;
    
    std::cout << "Scheduler::start() called" << std::endl;
    running_ = true;
    
    if (!GreenThread::getMainFiber()) {
        std::cout << "Converting main thread to fiber" << std::endl;
        LPVOID mainFiber = ConvertThreadToFiber(nullptr);
        if (!mainFiber) {
            DWORD error = GetLastError();
            if (error == ERROR_ALREADY_FIBER) {
                std::cout << "Thread is already a fiber, getting current fiber" << std::endl;
                mainFiber = GetCurrentFiber();
            } else {
                std::cerr << "Failed to convert main thread to fiber, error: " << error << std::endl;
                throw std::runtime_error("Failed to convert main thread to fiber");
            }
        }
        std::cout << "Setting main fiber: " << mainFiber << std::endl;
        GreenThread::setMainFiber(mainFiber);
    }
    
    if (!schedulerFiber_) {
        std::cout << "Creating scheduler fiber" << std::endl;
        schedulerFiber_ = CreateFiber(0, SchedulerFiberStart, this);
        if (!schedulerFiber_) {
            DWORD error = GetLastError();
            std::cerr << "Failed to create scheduler fiber, error: " << error << std::endl;
            throw std::runtime_error("Failed to create scheduler fiber");
        }
        std::cout << "Scheduler fiber created: " << schedulerFiber_ << std::endl;
    }
    
    std::cout << "Switching to scheduler fiber" << std::endl;
    SwitchToFiber(schedulerFiber_);
    std::cout << "Returned from scheduler fiber" << std::endl;
}

void Scheduler::run() {
    try {
        if (!schedulerFiber_) {
            std::cout << "Setting current fiber as scheduler fiber" << std::endl;
            schedulerFiber_ = GetCurrentFiber();
            if (!schedulerFiber_) {
                std::cerr << "ERROR: Failed to get current fiber" << std::endl;
                throw std::runtime_error("Failed to get current fiber");
            }
            std::cout << "Current fiber as scheduler: " << schedulerFiber_ << std::endl;
        }
        
        std::cout << "Entering scheduler loop" << std::endl;
        while (running_) {
            std::shared_ptr<GreenThread> thread = nullptr;
            bool needSleep = false;
            
            try {
                std::lock_guard<std::mutex> lock(queueMutex_);
                std::cout << "Queue size: " << readyQueue_.size() << ", Running threads: " << runningThreads_.size() << std::endl;
                
                if (readyQueue_.empty()) {
                    if (runningThreads_.empty()) {
                        std::cout << "No more threads to run, exiting scheduler" << std::endl;
                        break;
                    }
                    needSleep = true;
                    std::cout << "No ready threads, waiting..." << std::endl;
                } else {
                    thread = readyQueue_.front();
                    readyQueue_.pop_front();
                    runningThreads_.insert(thread);
                    std::cout << "Got thread ID " << thread->getId() << " from queue" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "ERROR in scheduler queue management: " << e.what() << std::endl;
                throw;
            }
            
            if (needSleep) {
                Sleep(1);
                continue;
            }
            
            if (thread) {
                try {
                    if (!thread->isFinished()) {
                        std::cout << "About to resume thread " << thread->getId() << std::endl;
                        currentThread_ = thread;
                        thread->resume();
                        std::cout << "Thread " << thread->getId() << " resumed and returned" << std::endl;
                        
                        if (thread->isFinished()) {
                            std::lock_guard<std::mutex> lock(queueMutex_);
                            runningThreads_.erase(thread);
                            std::cout << "Thread " << thread->getId() << " finished and removed from scheduler" << std::endl;
                        } else if (thread->getState() == GreenThread::State::READY) {
                            std::lock_guard<std::mutex> lock(queueMutex_);
                            readyQueue_.push_back(thread);
                            std::cout << "Thread " << thread->getId() << " yielded, putting back in queue" << std::endl;
                        } else {
                            std::cout << "Thread " << thread->getId() << " in state: " << static_cast<int>(thread->getState()) << std::endl;
                        }
                    } else {
                        std::lock_guard<std::mutex> lock(queueMutex_);
                        runningThreads_.erase(thread);
                        std::cout << "Thread " << thread->getId() << " was already finished, removing" << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "ERROR in thread execution: " << e.what() << std::endl;
                    throw;
                }
            }
            
            Sleep(0);
        }
        
        LPVOID mainFiber = GreenThread::getMainFiber();
        if (mainFiber) {
            std::cout << "Switching back to main fiber" << std::endl;
            SwitchToFiber(mainFiber);
            std::cout << "Returned from main fiber (this should not happen)" << std::endl;
        } else {
            std::cerr << "ERROR: No main fiber to return to!" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR in scheduler: " << e.what() << std::endl;
        
        LPVOID mainFiber = GreenThread::getMainFiber();
        if (mainFiber) {
            std::cerr << "Attempting emergency return to main fiber" << std::endl;
            SwitchToFiber(mainFiber);
        }
    } catch (...) {
        std::cerr << "UNKNOWN FATAL ERROR in scheduler" << std::endl;
        
        LPVOID mainFiber = GreenThread::getMainFiber();
        if (mainFiber) {
            std::cerr << "Attempting emergency return to main fiber" << std::endl;
            SwitchToFiber(mainFiber);
        }
    }
}

void WINAPI Scheduler::SchedulerFiberStart(LPVOID param) {
    Scheduler* scheduler = static_cast<Scheduler*>(param);
    if (scheduler) {
        scheduler->run();
    }
}

void Scheduler::stop() {
    running_ = false;
}

LPVOID Scheduler::getSchedulerFiber() const {
    return schedulerFiber_;
}

void Scheduler::yield() {
    auto thread = currentThread_.lock();
    if (thread) {
        thread->yield();
    } else if (schedulerFiber_) {
        if (GetCurrentFiber() != schedulerFiber_) {
            SwitchToFiber(schedulerFiber_);
        }
    }
}

} // namespace GreenThreads 