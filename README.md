<p align="center">
  <img src="https://github.com/user-attachments/assets/d5097020-2749-49de-a69a-4eadfe8aa3ad" alt="Green Threads Logo" width="800">
</p>

## Описание проекта

Green Threads Lite - это легковесная библиотека для реализации кооперативной многозадачности на основе "зеленых потоков" (green threads) с использованием Windows Fibers. Библиотека позволяет создавать множество легких потоков выполнения внутри одного системного потока и управлять их переключением.

## Основные компоненты

1. **GreenThread** - класс, представляющий зеленый поток. Внутренне использует Windows Fiber API.
2. **Scheduler** - планировщик, управляющий выполнением зеленых потоков и их переключением.
3. **Mutex** - примитив синхронизации для взаимоисключающего доступа между потоками.
4. **ConditionVariable** - примитив синхронизации для ожидания условий.

## Особенности

- Легковесная реализация многозадачности
- Кооперативное переключение потоков (не вытесняющее)
- Использует Windows Fiber API для эффективного переключения контекста
- Простой и интуитивно понятный API
- Минимальные зависимости

## Требования

- Windows операционная система
- Компилятор с поддержкой C++17 или выше
- CMake для сборки проекта

## Сборка проекта

1. Клонировать репозиторий:
```bash
git clone https://github.com/Danilka1337/Green-Threads-Lite.git
cd Green-Threads-Lite
```

2. Создать папку для сборки и собрать проект:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Использование библиотеки

### Включение заголовочных файлов

```cpp
#include <Scheduler.hpp>
#include <GreenThread.hpp>
#include <Mutex.hpp>
#include <ConditionVariable.hpp>
```

### Базовое использование

```cpp
// Получение экземпляра планировщика
auto& scheduler = GreenThreads::Scheduler::instance();

// Создание зеленого потока
auto thread = std::make_shared<GreenThreads::GreenThread>([]() {
    // Функция, выполняемая в потоке
    std::cout << "Hello from green thread!" << std::endl;
    
    // Передача управления другим потокам
    GreenThreads::Scheduler::instance().yield();
    
    std::cout << "Green thread continues after yield" << std::endl;
});

// Запуск потока
thread->start();

// Запуск планировщика
scheduler.run();
```

### Синхронизация с Mutex

```cpp
GreenThreads::Mutex mutex;

// Блокирующий захват
mutex.lock();
// Критическая секция
mutex.unlock();

// С помощью RAII
{
    std::lock_guard<GreenThreads::Mutex> lock(mutex);
    // Критическая секция
}
```

### Условные переменные

```cpp
GreenThreads::Mutex mutex;
GreenThreads::ConditionVariable cv;

// Ожидание события
{
    std::unique_lock<GreenThreads::Mutex> lock(mutex);
    cv.wait(lock); // Разблокирует mutex и ждет, пока не будет вызван notify
}

// Ожидание с таймаутом
{
    std::unique_lock<GreenThreads::Mutex> lock(mutex);
    bool notified = cv.wait_for(lock, std::chrono::milliseconds(500));
    if (!notified) {
        // Истек таймаут, событие не произошло
    }
}

// Уведомление потоков
cv.notify_one(); // Разбудить один поток
cv.notify_all(); // Разбудить все ожидающие потоки
```

## Пример: Производитель-Потребитель

```cpp
#include <Scheduler.hpp>
#include <GreenThread.hpp>
#include <ConditionVariable.hpp>
#include <Mutex.hpp>
#include <iostream>
#include <queue>

using namespace GreenThreads;

// Общие ресурсы
std::queue<int> dataQueue;
Mutex queueMutex;
ConditionVariable queueNotEmpty;
ConditionVariable queueNotFull;
constexpr int MAX_QUEUE_SIZE = 5;

// Производитель
void producer() {
    for (int i = 1; i <= 10; ++i) {
        {
            std::unique_lock<Mutex> lock(queueMutex);
            
            // Ждем, если очередь полна
            while (dataQueue.size() >= MAX_QUEUE_SIZE) {
                queueNotFull.wait(lock);
            }
            
            // Добавляем данные
            dataQueue.push(i);
            std::cout << "Produced: " << i << std::endl;
            
            // Уведомляем потребителя
            queueNotEmpty.notify_one();
        }
        
        // Уступаем управление
        Scheduler::instance().yield();
    }
}

// Потребитель
void consumer() {
    for (int i = 0; i < 10; ++i) {
        int data;
        
        {
            std::unique_lock<Mutex> lock(queueMutex);
            
            // Ждем данных
            while (dataQueue.empty()) {
                queueNotEmpty.wait(lock);
            }
            
            // Извлекаем данные
            data = dataQueue.front();
            dataQueue.pop();
            
            // Уведомляем производителя
            queueNotFull.notify_one();
        }
        
        // Обрабатываем данные
        std::cout << "Consumed: " << data << std::endl;
        
        // Уступаем управление
        Scheduler::instance().yield();
    }
}

int main() {
    auto& scheduler = Scheduler::instance();
    
    // Создаем и запускаем потоки
    auto producerThread = std::make_shared<GreenThread>(producer);
    auto consumerThread = std::make_shared<GreenThread>(consumer);
    
    producerThread->start();
    consumerThread->start();
    
    // Запускаем планировщик
    scheduler.run();
    
    return 0;
}
```

## Принципы работы библиотеки

1. **Кооперативная многозадачность**: Потоки должны явно вызывать `yield()` для передачи управления другим потокам.
2. **Планирование потоков**: Потоки помещаются в очередь готовых к выполнению, и планировщик управляет их выполнением.
3. **Fiber API**: Внутренне потоки реализованы с использованием Windows Fiber API, что обеспечивает эффективное переключение контекста.
4. **Синхронизация**: Библиотека предоставляет примитивы синхронизации (`Mutex`, `ConditionVariable`).

## Ограничения

1. Работает только на Windows из-за использования Windows Fiber API
2. Кооперативная многозадачность требует явного вызова `yield()` для передачи управления
3. Блокирующие операции в потоке блокируют все потоки
4. Не рекомендуется использовать для задач, требующих интенсивных вычислений без частого yield

## Советы по использованию

- Регулярно вызывайте `yield()` в зеленых потоках, чтобы обеспечить плавное переключение
- Избегайте длительных блокирующих операций
- Используйте мьютексы для синхронизации доступа к общим ресурсам
- Для задач ввода-вывода рассмотрите асинхронные операции с коллбэками



