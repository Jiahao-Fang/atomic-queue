cmake_minimum_required(VERSION 3.10)
project(atomic_queue)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set optimization level to O3 and add necessary flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wno-interference-size")

# Add executable targets
add_executable(mpmc_example src/mpmc_example.cpp)
add_executable(spmc_example src/spmc_example.cpp)

# Add include directories
target_include_directories(mpmc_example PRIVATE include)
target_include_directories(spmc_example PRIVATE include)

# Add compile definitions for cache line size
target_compile_definitions(mpmc_example PRIVATE CACHE_LINE_SIZE=64)
target_compile_definitions(spmc_example PRIVATE CACHE_LINE_SIZE=64) 