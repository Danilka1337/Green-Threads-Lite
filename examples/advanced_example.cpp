#include <Scheduler.hpp>
#include <GreenThread.hpp>
#include <ConditionVariable.hpp>
#include <Mutex.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <queue>
#include <thread>

using namespace GreenThreads;
using namespace std::chrono_literals;

std::mutex coutMutex;

void safePrint(const std::string& message) {
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << message << std::endl;
}

std::queue<int> dataQueue;
Mutex queueMutex;
ConditionVariable queueNotEmpty;
ConditionVariable queueNotFull;
constexpr int MAX_QUEUE_SIZE = 5;
bool producerFinished = false;

void producer() {
    try {
        safePrint("Producer: Started");
        
        for (int i = 1; i <= 10; ++i) {
            std::this_thread::sleep_for(100ms);
            
            {
                std::unique_lock<Mutex> lock(queueMutex);
                
                while (dataQueue.size() >= MAX_QUEUE_SIZE) {
                    safePrint("Producer: Queue full, waiting...");
                    queueNotFull.wait(lock);
                }
                
                dataQueue.push(i);
                safePrint("Producer: Added item " + std::to_string(i));
                
                queueNotEmpty.notify_one();
            }
            
            Scheduler::instance().yield();
        }
        
        {
            std::lock_guard<Mutex> lock(queueMutex);
            producerFinished = true;
            queueNotEmpty.notify_one();
        }
        
        safePrint("Producer: Finished");
    } catch (const std::exception& e) {
        safePrint("Producer EXCEPTION: " + std::string(e.what()));
    } catch (...) {
        safePrint("Producer UNKNOWN EXCEPTION");
    }
}

void consumer() {
    try {
        safePrint("Consumer: Started");
        
        while (true) {
            int data = 0;
            bool hasData = false;
            
            {
                std::unique_lock<Mutex> lock(queueMutex);
                
                if (dataQueue.empty() && producerFinished) {
                    break;
                }
                
                if (dataQueue.empty()) {
                    safePrint("Consumer: Waiting for data...");
                    bool notified = queueNotEmpty.wait_for(lock, 500ms);
                    
                    if (!notified) {
                        safePrint("Consumer: Timeout waiting for data");
                        continue;
                    }
                    
                    if (dataQueue.empty() && producerFinished) {
                        break;
                    }
                }
                
                if (!dataQueue.empty()) {
                    data = dataQueue.front();
                    dataQueue.pop();
                    hasData = true;
                    
                    queueNotFull.notify_one();
                }
            }
            
            if (hasData) {
                safePrint("Consumer: Processing item " + std::to_string(data));
                std::this_thread::sleep_for(150ms);
            }
            
            Scheduler::instance().yield();
        }
        
        safePrint("Consumer: Finished");
    } catch (const std::exception& e) {
        safePrint("Consumer EXCEPTION: " + std::string(e.what()));
    } catch (...) {
        safePrint("Consumer UNKNOWN EXCEPTION");
    }
}

int main() {
    try {
        safePrint("Main: Initializing");
        
        auto& scheduler = Scheduler::instance();
        
        safePrint("Main: Creating producer thread");
        auto producerThread = std::make_shared<GreenThread>(producer);
        safePrint("Main: Creating consumer thread");
        auto consumerThread = std::make_shared<GreenThread>(consumer);
        
        safePrint("Main: Starting producer thread");
        producerThread->start();
        safePrint("Main: Starting consumer thread");
        consumerThread->start();
        
        safePrint("Main: Starting scheduler...");
        scheduler.start();
        safePrint("Main: Starting scheduler...");
        scheduler.start();
        safePrint("Main: All threads completed");
        
        return 0;
    } catch (const std::exception& e) {
        safePrint("MAIN EXCEPTION: " + std::string(e.what()));
        return 1;
    } catch (...) {
        safePrint("MAIN UNKNOWN EXCEPTION");
        return 2;
    }
} 