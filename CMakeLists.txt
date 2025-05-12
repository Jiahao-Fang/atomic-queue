cmake_minimum_required(VERSION 3.10)
project(atomic_queue)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译优化级别为O3
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# 添加可执行文件
add_executable(performance_test src/performance_test.cpp)

# 添加头文件目录
target_include_directories(performance_test PRIVATE include) 