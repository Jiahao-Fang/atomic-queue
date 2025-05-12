#include <gtest/gtest.h>
#include "../include/atomic_queue.hpp"

// 测试 HeapBuffer 的基本功能
TEST(HeapBufferTest, BasicOperations) {
    sl::HeapBuffer<int, sl::EnablePowerOfTwo, true, 4> buffer(4);
    
    // 测试索引访问
    buffer[0] = 1;
    buffer[1] = 2;
    EXPECT_EQ(buffer[0], 1);
    EXPECT_EQ(buffer[1], 2);
    
    // 测试模运算
    EXPECT_EQ(buffer[4], buffer[0]); // 4 % 4 = 0
    EXPECT_EQ(buffer[5], buffer[1]); // 5 % 4 = 1
}

// 测试 HeapBuffer 的模运算功能
TEST(HeapBufferTest, ModuloOperations) {
    sl::HeapBuffer<int, sl::EnablePowerOfTwo, true, 8> buffer(8);
    
    // 填充缓冲区
    for (int i = 0; i < 8; ++i) {
        buffer[i] = i;
    }
    
    // 验证模运算
    for (int i = 8; i < 16; ++i) {
        EXPECT_EQ(buffer[i], buffer[i % 8]);
    }
}

// 测试 HeapBuffer 的边界情况
TEST(HeapBufferTest, BoundaryConditions) {
    sl::HeapBuffer<int, sl::EnablePowerOfTwo, true, 4> buffer(4);
    
    // 测试边界访问
    EXPECT_NO_THROW(buffer[3]);
    EXPECT_NO_THROW(buffer[7]); // 7 % 4 = 3
}

// 测试 HeapBuffer 的分配器
TEST(HeapBufferTest, Allocator) {
    std::allocator<int> custom_allocator;
    sl::HeapBuffer<int, sl::EnablePowerOfTwo, true, 4> buffer(4, custom_allocator);
    
    // 验证缓冲区可以正常使用
    buffer[0] = 42;
    EXPECT_EQ(buffer[0], 42);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 