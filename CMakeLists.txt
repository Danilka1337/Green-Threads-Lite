cmake_minimum_required(VERSION 3.10)
project(GreenThreadsLite VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(GreenThreads
    src/GreenThread.cpp
    src/Scheduler.cpp
)

target_include_directories(GreenThreads
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(simple_test examples/simple_test.cpp)
target_link_libraries(simple_test GreenThreads)

install(TARGETS GreenThreads
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
) 