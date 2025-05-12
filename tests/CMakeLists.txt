cmake_minimum_required(VERSION 3.14)
project(atomic_queue_tests)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加 Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
FetchContent_MakeAvailable(googletest)

# 添加测试可执行文件
add_executable(cell_test cell_test.cpp)
add_executable(heap_buffer_test heap_buffer_test.cpp)
add_executable(stack_buffer_test stack_buffer_test.cpp)
add_executable(mpmc_queue_test mpmc_queue_test.cpp)

# 链接 Google Test
target_link_libraries(cell_test gtest_main)
target_link_libraries(heap_buffer_test gtest_main)
target_link_libraries(stack_buffer_test gtest_main)
target_link_libraries(mpmc_queue_test gtest_main)

# 启用测试
enable_testing()
add_test(NAME cell_test COMMAND cell_test)
add_test(NAME heap_buffer_test COMMAND heap_buffer_test)
add_test(NAME stack_buffer_test COMMAND stack_buffer_test)
add_test(NAME mpmc_queue_test COMMAND mpmc_queue_test) 