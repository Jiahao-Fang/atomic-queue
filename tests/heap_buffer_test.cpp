#include <gtest/gtest.h>
#include "../include/atomic_queue.hpp"

// Test basic functionality of HeapBuffer
TEST(HeapBufferTest, BasicOperations) {
    sl::HeapBuffer<int, sl::EnablePowerOfTwo, true, 4> buffer(4);
    
    // Test index access
    buffer[0] = 1;
    buffer[1] = 2;
    EXPECT_EQ(buffer[0], 1);
    EXPECT_EQ(buffer[1], 2);
    
    // Test modulo operations
    EXPECT_EQ(buffer[4], buffer[0]); // 4 % 4 = 0
    EXPECT_EQ(buffer[5], buffer[1]); // 5 % 4 = 1
}

// Test HeapBuffer's modulo operations
TEST(HeapBufferTest, ModuloOperations) {
    sl::HeapBuffer<int, sl::EnablePowerOfTwo, true, 8> buffer(8);
    
    // Fill the buffer
    for (int i = 0; i < 8; ++i) {
        buffer[i] = i;
    }
    
    // Verify modulo operations
    for (int i = 8; i < 16; ++i) {
        EXPECT_EQ(buffer[i], buffer[i % 8]);
    }
}

// Test HeapBuffer's boundary conditions
TEST(HeapBufferTest, BoundaryConditions) {
    sl::HeapBuffer<int, sl::EnablePowerOfTwo, true, 4> buffer(4);
    
    // Test boundary access
    EXPECT_NO_THROW(buffer[3]);
    EXPECT_NO_THROW(buffer[7]); // 7 % 4 = 3
}

// Test HeapBuffer's allocator
TEST(HeapBufferTest, Allocator) {
    std::allocator<int> custom_allocator;
    sl::HeapBuffer<int, sl::EnablePowerOfTwo, true, 4> buffer(4, custom_allocator);
    
    // Verify buffer can be used normally
    buffer[0] = 42;
    EXPECT_EQ(buffer[0], 42);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 