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

// Мьютекс для синхронизации вывода в консоль
std::mutex coutMutex;

// Функция для безопасного вывода в консоль
void safePrint(const std::string& message) {
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << message << std::endl;
}

std::queue<int> dataQueue;           // Очередь данных
Mutex queueMutex;                    // Мьютекс для защиты очереди
ConditionVariable queueNotEmpty;     // Условная переменная для сигнала о наличии данных
ConditionVariable queueNotFull;      // Условная переменная для сигнала о свободном месте
constexpr int MAX_QUEUE_SIZE = 5;    // Максимальный размер очереди
bool producerFinished = false;       // Флаг завершения producer

// генерирует данные
void producer() {
    try {
        safePrint("Producer: Started");
        
        // Генерация 10 элементов
        for (int i = 1; i <= 10; ++i) {
            std::this_thread::sleep_for(100ms);  // Имитация работы
            
            {
                // Захватываем мьютекс для работы с очередью
                std::unique_lock<Mutex> lock(queueMutex);
                
                // Ждем пока в очереди не будет места
                while (dataQueue.size() >= MAX_QUEUE_SIZE) {
                    safePrint("Producer: Queue full, waiting...");
                    queueNotFull.wait(lock);  // Освобождает мьютекс и ждет сигнала
                }
                
                // Добавляем данные в очередь
                dataQueue.push(i);
                safePrint("Producer: Added item " + std::to_string(i));
                
                // Сигнализируем consumer о новых данных
                queueNotEmpty.notify_one();
            }
            
            // Передаем управление другим потокам
            Scheduler::instance().yield();
        }
        
        {
            // Сообщаем о завершении работы
            std::lock_guard<Mutex> lock(queueMutex);
            producerFinished = true;
            queueNotEmpty.notify_one();  // Будим consumer для завершения
        }
        
        safePrint("Producer: Finished");
    } catch (const std::exception& e) {
        safePrint("Producer EXCEPTION: " + std::string(e.what()));
    } catch (...) {
        safePrint("Producer UNKNOWN EXCEPTION");
    }
}

// Функция consumerобрабатывает данные
void consumer() {
    try {
        safePrint("Consumer: Started");
        
        while (true) {
            int data = 0;
            bool hasData = false;
            
            {
                // Захватываем мьютекс для работы с очередью
                std::unique_lock<Mutex> lock(queueMutex);
                
                // Проверяем условия завершения
                if (dataQueue.empty() && producerFinished) {
                    break;
                }
                
                // Если очередь пуста, ждем данных
                if (dataQueue.empty()) {
                    safePrint("Consumer: Waiting for data...");
                    // Ждем с таймаутом 500мс
                    bool notified = queueNotEmpty.wait_for(lock, 500ms);
                    
                    if (!notified) {
                        safePrint("Consumer: Timeout waiting for data");
                        continue;
                    }
                    
                    // Проверяем условия завершения после ожидания
                    if (dataQueue.empty() && producerFinished) {
                        break;
                    }
                }
                
                // Если есть данные извлекаем их
                if (!dataQueue.empty()) {
                    data = dataQueue.front();
                    dataQueue.pop();
                    hasData = true;
                    
                    // Сигнализируем producer о свободном месте
                    queueNotFull.notify_one();
                }
            }
            
            // Обрабатываем данные
            if (hasData) {
                safePrint("Consumer: Processing item " + std::to_string(data));
                std::this_thread::sleep_for(150ms);  // Имитация обработки
            }
            
            // Передаем управление другим потокам
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
        
        // Получаем экземпляр планировщика
        auto& scheduler = Scheduler::instance();
        
        // Создаем потоки producer и consumer
        safePrint("Main: Creating producer thread");
        auto producerThread = std::make_shared<GreenThread>(producer);
        safePrint("Main: Creating consumer thread");
        auto consumerThread = std::make_shared<GreenThread>(consumer);
        
        // Запускаем потоки
        safePrint("Main: Starting producer thread");
        producerThread->start();
        safePrint("Main: Starting consumer thread");
        consumerThread->start();
        
        // Запускаем планировщик
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