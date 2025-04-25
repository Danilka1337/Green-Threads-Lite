cmake_minimum_required(VERSION 3.10)
project(GreenThreadsExample)

link_directories(${CMAKE_BINARY_DIR}/../build)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(advanced_example advanced_example.cpp)
target_link_libraries(advanced_example GreenThreads) 