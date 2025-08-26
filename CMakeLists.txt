# Makefile for train2_optimized_drive program

cmake_minimum_required(VERSION 3.10)
project(spingo_train)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)  # Updated to C++17 for better thread support
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -pthread")  # Added optimization and thread support

# Spingo source and include directories
set(SPINGO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/spingo")
set(SPINGO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/spingo")

# Create spingo library from source
add_library(spingo STATIC 
    ${SPINGO_SRC_DIR}/spingo.cpp
    # Add any other source files if they exist
)

# Set include directories for spingo library
target_include_directories(spingo PUBLIC ${SPINGO_INCLUDE_DIR})

# Add the executable
add_executable(train2_optimized_drive train2_optimized_drive.cpp)

# Include directories for train2_optimized_drive
target_include_directories(train2_optimized_drive PRIVATE ${SPINGO_INCLUDE_DIR})

# Link libraries
target_link_libraries(train2_optimized_drive spingo pthread)  # Added pthread for thread support

# Add a clean target (optional, as CMake provides this automatically)
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_clean.cmake"
    COMMENT "Cleaning all build outputs"
)