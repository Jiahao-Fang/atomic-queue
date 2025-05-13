#include <gtest/gtest.h>
#include "../include/atomic_queue.hpp"
#include <string>

// Test Cell with Trivial Type
TEST(CellTest, TrivialType) {
    sl::Cell<int, true> cell;
    
    // Test construction and reading
    cell.construct(42);
    EXPECT_EQ(cell.read(), 42);
    
    // Test sequence number
    EXPECT_EQ(cell.seq_.load(), 0);
}

// Test Cell with Non-Trivial Type
TEST(CellTest, NonTrivialType) {
    sl::Cell<std::string, true> cell;
    
    // Test construction and reading
    std::string str = "test";
    cell.construct(str);
    EXPECT_EQ(cell.read(), "test");
    
    // Test destruction
    cell.destroy();
}

// Test Cell's sequence number functionality
TEST(CellTest, SequenceNumber) {
    sl::Cell<int, true> cell(42);
    EXPECT_EQ(cell.seq_.load(), 42);
    
    // Test sequence number update
    cell.seq_.store(43);
    EXPECT_EQ(cell.seq_.load(), 43);
}

// Test Cell's memory alignment
TEST(CellTest, MemoryAlignment) {
    static_assert(alignof(sl::Cell<int, true>) >= sl::cache_line, "Cell should be cache line aligned");
    static_assert(alignof(sl::Cell<std::string, true>) >= sl::cache_line, "Cell should be cache line aligned");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 