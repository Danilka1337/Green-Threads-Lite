#include "GreenThread.hpp"
#include "Scheduler.hpp"
#include <iostream>
#include <stdexcept>

namespace GreenThreads {

LPVOID GreenThread::mainFiber_ = nullptr;
thread_local std::weak_ptr<GreenThread> GreenThread::currentThread_;
static std::atomic<int> nextId = 0;

GreenThread::GreenThread(ThreadFunction func)
    : function_(std::move(func)), 
      fiber_(nullptr),
      state_(State::READY),
      id_(nextId++) {
}

GreenThread::~GreenThread() {
    if (fiber_ && GetCurrentFiber() != fiber_) {
        DeleteFiber(fiber_);
        fiber_ = nullptr;
    }
}

void GreenThread::start() {
    if (state_ != State::READY) {
        return;
    }

    if (!fiber_) {
        fiber_ = CreateFiber(0, FiberStart, this);
        if (!fiber_) {
            throw std::runtime_error("Failed to create fiber for green thread");
        }
    }

    std::cout << "Starting thread " << id_ << std::endl;
    Scheduler::instance().addThread(shared_from_this());
}

void GreenThread::resume() {
    try {
        if (state_ == State::FINISHED) {
            std::cout << "Not resuming finished thread " << id_ << std::endl;
            return;
        }

        if (!fiber_) {
            std::cerr << "ERROR: Cannot resume thread " << id_ << " with no fiber" << std::endl;
            throw std::runtime_error("Cannot resume thread with no fiber");
        }

        LPVOID currentFiber = GetCurrentFiber();
        if (!currentFiber || currentFiber == (void*)0x1E00) {
            std::cerr << "ERROR: Cannot resume thread " << id_ << " from a non-fiber context" << std::endl;
            throw std::runtime_error("Cannot resume thread from a non-fiber context");
        }

        std::cout << "Thread " << id_ << " being resumed, current fiber: " << currentFiber << ", target fiber: " << fiber_ << std::endl;
        
        previousFiber_ = currentFiber;
        currentThread_ = shared_from_this();
        state_ = State::RUNNING;
        
        std::cout << "Switching to thread " << id_ << "'s fiber" << std::endl;
        
        SwitchToFiber(fiber_);
        
        std::cout << "Returned to thread " << id_ << " from fiber, state: " << 
            (state_ == State::FINISHED ? "FINISHED" : 
            state_ == State::READY ? "READY" : 
            state_ == State::RUNNING ? "RUNNING" : 
            state_ == State::SUSPENDED ? "SUSPENDED" : "UNKNOWN") << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR in thread " << id_ << " resume: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "UNKNOWN ERROR in thread " << id_ << " resume" << std::endl;
        throw;
    }
}

void GreenThread::yield() {
    std::cout << "Thread " << id_ << " yielding" << std::endl;
    
    if (!previousFiber_) {
        LPVOID schedulerFiber = Scheduler::instance().getSchedulerFiber();
        if (schedulerFiber) {
            previousFiber_ = schedulerFiber;
        } else {
            throw std::runtime_error("No scheduler fiber to return to!");
        }
    }

    if (state_ == State::RUNNING) {
        state_ = State::READY;
    }

    auto previousThread = currentThread_;
    currentThread_.reset();
    LPVOID fiberToSwitchTo = previousFiber_;
    previousFiber_ = nullptr;
    SwitchToFiber(fiberToSwitchTo);
}

void WINAPI GreenThread::FiberStart(LPVOID param) {
    try {
        std::cout << "FiberStart: Function entered with param: " << param << std::endl;
        
        auto* thread = static_cast<GreenThread*>(param);
        if (!thread) {
            std::cerr << "ERROR: FiberStart received null thread parameter" << std::endl;
            LPVOID mainFiber = GreenThread::mainFiber_;
            if (mainFiber) {
                std::cerr << "Attempting emergency return to main fiber" << std::endl;
                SwitchToFiber(mainFiber);
            } else {
                std::cerr << "No main fiber to return to, cannot recover" << std::endl;
            }
            return;
        }

        std::cout << "Starting fiber for thread " << thread->getId() << std::endl;
        
        try {
            std::cout << "FiberStart: About to run thread function for thread " << thread->getId() << std::endl;
            thread->run();
            std::cout << "FiberStart: Thread function completed normally for thread " << thread->getId() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception in thread " << thread->getId() << ": " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception in thread " << thread->getId() << std::endl;
        }

        std::cout << "Thread " << thread->getId() << " function completed, marking as FINISHED" << std::endl;
        thread->state_ = State::FINISHED;

        LPVOID fiberToSwitchTo = thread->previousFiber_;
        if (!fiberToSwitchTo) {
            std::cout << "No previous fiber, trying scheduler fiber" << std::endl;
            fiberToSwitchTo = Scheduler::instance().getSchedulerFiber();
            
            if (!fiberToSwitchTo) {
                std::cout << "No scheduler fiber, trying main fiber" << std::endl;
                fiberToSwitchTo = mainFiber_;
            }
            
            if (!fiberToSwitchTo) {
                std::cerr << "ERROR: No fiber to return to!" << std::endl;
                std::terminate();
            }
        }
        
        std::cout << "FiberStart: Thread " << thread->getId() << " clearing current thread reference" << std::endl;
        
        thread->currentThread_.reset();
        
        std::cout << "Thread " << thread->getId() << " switching back to fiber: " << fiberToSwitchTo << std::endl;
        
        SwitchToFiber(fiberToSwitchTo);
        
        std::cerr << "ERROR: Thread " << thread->getId() << " returned from SwitchToFiber!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR in fiber start: " << e.what() << std::endl;
        LPVOID mainFiber = GreenThread::mainFiber_;
        if (mainFiber) {
            std::cerr << "Attempting emergency return to main fiber" << std::endl;
            SwitchToFiber(mainFiber);
        }
    } catch (...) {
        std::cerr << "UNKNOWN FATAL ERROR in fiber start" << std::endl;
        LPVOID mainFiber = GreenThread::mainFiber_;
        if (mainFiber) {
            std::cerr << "Attempting emergency return to main fiber" << std::endl;
            SwitchToFiber(mainFiber);
        }
    }
}

void GreenThread::run() {
    if (function_) {
        function_();
    }
}

int GreenThread::getId() const {
    return id_;
}

bool GreenThread::isFinished() const {
    return state_ == State::FINISHED;
}

void GreenThread::setMainFiber(LPVOID fiber) {
    mainFiber_ = fiber;
}

LPVOID GreenThread::getMainFiber() {
    return mainFiber_;
}

std::shared_ptr<GreenThread> GreenThread::current() {
    return currentThread_.lock();
}

} // namespace GreenThreads 