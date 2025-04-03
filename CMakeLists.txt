cmake_minimum_required(VERSION 3.10)
project(spingo)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# List all source files and define the executable target
add_executable(spingo main.cpp spingo/spingo.cpp spingo/mccfr.cpp)

# Add include directories - point to the spingo directory
target_include_directories(spingo PRIVATE spingo)