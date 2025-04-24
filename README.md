# Green-Threads-lite

## Описание проекта

Green-Threads-lite - это легковесная библиотека для реализации кооперативной многозадачности на основе "зеленых потоков" (green threads) с использованием Windows Fibers. Библиотека позволяет создавать множество легких потоков выполнения внутри одного системного потока и управлять их переключением.

## Основные компоненты

1. **GreenThread** - класс, представляющий зеленый поток. Внутренне использует Windows Fiber API.
2. **Scheduler** - планировщик, управляющий выполнением зеленых потоков и их переключением.

## Особенности

- Легковесная реализация многозадачности
- Кооперативное переключение потоков (не вытесняющее)
- Использует Windows Fiber API для эффективного переключения контекста
- Простой и интуитивно понятный API
- Минимальные зависимости

## Требования

- Windows операционная система
- Компилятор с поддержкой C++11 или выше
- CMake для сборки проекта

## Сборка проекта

1. Клонировать репозиторий:
```bash
git clone <url-репозитория>
cd Green-Threads-lite
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
```

### Создание и запуск зеленых потоков

```cpp
// Получение экземпляра планировщика
auto& scheduler = GreenThreads::Scheduler::getInstance();

// Создание зеленого потока
scheduler.spawn([]() {
    // Функция, выполняемая в потоке
    std::cout << "Hello from green thread!" << std::endl;
    
    // Передача управления другим потокам
    GreenThreads::Scheduler::getInstance().yield();
    
    std::cout << "Green thread continues after yield" << std::endl;
});

// Запуск планировщика (блокирует текущий поток до завершения всех зеленых потоков)
scheduler.run();
```

### Пример использования

```cpp
#include <Scheduler.hpp>
#include <iostream>
#include <mutex>
#include <string>
#include <chrono>
#include <thread>

using namespace GreenThreads;

// Мьютекс для синхронизированного вывода
std::mutex coutMutex;

void safePrint(const std::string& message) {
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << message << std::endl;
}

void worker(int id) {
    safePrint("Worker " + std::to_string(id) + ": Started");
    
    for (int i = 0; i < 3; ++i) {
        safePrint("Worker " + std::to_string(id) + ": Step " + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        Scheduler::getInstance().yield();
    }
    
    safePrint("Worker " + std::to_string(id) + ": Finished");
}

int main() {
    auto& scheduler = Scheduler::getInstance();

    // Создание нескольких рабочих потоков
    for (int i = 0; i < 3; ++i) {
        scheduler.spawn([i]() { worker(i); });
    }

    safePrint("Starting scheduler...");
    scheduler.run();
    safePrint("All threads completed");

    return 0;
}
```

## Принципы работы библиотеки

1. **Кооперативная многозадачность**: Потоки должны явно вызывать `yield()` для передачи управления другим потокам.
2. **Планирование потоков**: Потоки помещаются в очередь готовых к выполнению, и планировщик управляет их выполнением.
3. **Fiber API**: Внутренне потоки реализованы с использованием Windows Fiber API, что обеспечивает эффективное переключение контекста.

## Архитектура

### GreenThread

Класс `GreenThread` представляет собой зеленый поток и инкапсулирует:
- Функцию, выполняемую в потоке (`func_`)
- Windows Fiber для переключения контекста (`fiber_`)
- Состояние завершения (`finished_`)

### Scheduler

Класс `Scheduler` управляет зелеными потоками:
- Создает и хранит потоки
- Поддерживает очередь готовых к выполнению потоков
- Управляет переключением между потоками
- Реализует паттерн Singleton для глобального доступа

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

