cmake_minimum_required(VERSION 3.1)

project(asynclogger)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} async_logger.cpp test.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
