#include <gtest/gtest.h>
#include "../include/atomic_queue.hpp"

// Test basic functionality of StackBuffer
TEST(StackBufferTest, BasicOperations) {
    sl::StackBuffer<int, true, 4> buffer(nullptr, nullptr);
    
    // Test index access
    buffer[0] = 1;
    buffer[1] = 2;
    EXPECT_EQ(buffer[0], 1);
    EXPECT_EQ(buffer[1], 2);
    
    // Test modulo operations
    EXPECT_EQ(buffer[4], buffer[0]); // 4 % 4 = 0
    EXPECT_EQ(buffer[5], buffer[1]); // 5 % 4 = 1
}

// Test StackBuffer's modulo operations
TEST(StackBufferTest, ModuloOperations) {
    sl::StackBuffer<int, true, 8> buffer(nullptr, nullptr);
    
    // Fill the buffer
    for (int i = 0; i < 8; ++i) {
        buffer[i] = i;
    }
    
    // Verify modulo operations
    for (int i = 8; i < 16; ++i) {
        EXPECT_EQ(buffer[i], buffer[i % 8]);
    }
}

// Test StackBuffer's boundary conditions
TEST(StackBufferTest, BoundaryConditions) {
    sl::StackBuffer<int, true, 4> buffer(nullptr, nullptr);
    
    // Test boundary access
    EXPECT_NO_THROW(buffer[3]);
    EXPECT_NO_THROW(buffer[7]); // 7 % 4 = 3
}

// Test StackBuffer's compile-time optimization
TEST(StackBufferTest, CompileTimeOptimization) {
    // Verify StackBuffer size is a compile-time constant
    static_assert(sizeof(sl::StackBuffer<int, true, 4>) == sizeof(int) * 4,
                 "StackBuffer size should be known at compile time");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 