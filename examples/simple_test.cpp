#include <Scheduler.hpp>
#include <iostream>
#include <mutex>
#include <string>
#include <chrono>
#include <thread>

using namespace GreenThreads;

// Mutex for synchronized output
std::mutex coutMutex;

void safePrint(const std::string& message) {
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << message << std::endl;
}

void worker(int id) {
    safePrint("Worker " + std::to_string(id) + ": Started");
    
    for (int i = 0; i < 3; ++i) {
        safePrint("Worker " + std::to_string(id) + ": Step " + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Smaller delay
        Scheduler::getInstance().yield();
    }
    
    safePrint("Worker " + std::to_string(id) + ": Finished");
}

int main() {
    auto& scheduler = Scheduler::getInstance();

    // Spawn multiple workers
    for (int i = 0; i < 3; ++i) {
        scheduler.spawn([i]() { worker(i); });
    }

    safePrint("Starting scheduler...");
    scheduler.run();
    safePrint("All threads completed");

    return 0;
} 