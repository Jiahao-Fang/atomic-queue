#include <gtest/gtest.h>
#include "../include/atomic_queue.hpp"

// 测试 StackBuffer 的基本功能
TEST(StackBufferTest, BasicOperations) {
    sl::StackBuffer<int, true, 4> buffer(nullptr, nullptr);
    
    // 测试索引访问
    buffer[0] = 1;
    buffer[1] = 2;
    EXPECT_EQ(buffer[0], 1);
    EXPECT_EQ(buffer[1], 2);
    
    // 测试模运算
    EXPECT_EQ(buffer[4], buffer[0]); // 4 % 4 = 0
    EXPECT_EQ(buffer[5], buffer[1]); // 5 % 4 = 1
}

// 测试 StackBuffer 的模运算功能
TEST(StackBufferTest, ModuloOperations) {
    sl::StackBuffer<int, true, 8> buffer(nullptr, nullptr);
    
    // 填充缓冲区
    for (int i = 0; i < 8; ++i) {
        buffer[i] = i;
    }
    
    // 验证模运算
    for (int i = 8; i < 16; ++i) {
        EXPECT_EQ(buffer[i], buffer[i % 8]);
    }
}

// 测试 StackBuffer 的边界情况
TEST(StackBufferTest, BoundaryConditions) {
    sl::StackBuffer<int, true, 4> buffer(nullptr, nullptr);
    
    // 测试边界访问
    EXPECT_NO_THROW(buffer[3]);
    EXPECT_NO_THROW(buffer[7]); // 7 % 4 = 3
}

// 测试 StackBuffer 的编译时优化
TEST(StackBufferTest, CompileTimeOptimization) {
    // 验证 StackBuffer 的大小是编译时常量
    static_assert(sizeof(sl::StackBuffer<int, true, 4>) == sizeof(int) * 4,
                 "StackBuffer size should be known at compile time");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 