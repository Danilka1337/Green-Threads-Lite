cmake_minimum_required(VERSION 3.10)
project(GreenThreadsExample)

# Добавляем путь к библиотеке
link_directories(${CMAKE_BINARY_DIR}/../build)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Создаем исполняемый файл
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example GreenThreads)

add_executable(sync_example sync_example.cpp)
target_link_libraries(sync_example GreenThreads)

add_executable(complex_example complex_example.cpp)
target_link_libraries(complex_example GreenThreads)

add_executable(demo_example demo_example.cpp)
target_link_libraries(demo_example GreenThreads)

add_executable(simple_test simple_test.cpp)
target_link_libraries(simple_test GreenThreads) 