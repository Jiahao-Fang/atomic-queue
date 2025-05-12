#include <gtest/gtest.h>
#include "../include/atomic_queue.hpp"
#include <string>

// 测试平凡类型（Trivial Type）的 Cell
TEST(CellTest, TrivialType) {
    sl::Cell<int, true> cell;
    
    // 测试构造和读取
    cell.construct(42);
    EXPECT_EQ(cell.read(), 42);
    
    // 测试序列号
    EXPECT_EQ(cell.seq_.load(), 0);
}

// 测试非平凡类型（Non-Trivial Type）的 Cell
TEST(CellTest, NonTrivialType) {
    sl::Cell<std::string, true> cell;
    
    // 测试构造和读取
    std::string str = "test";
    cell.construct(str);
    EXPECT_EQ(cell.read(), "test");
    
    // 测试析构
    cell.destroy();
}

// 测试 Cell 的序列号功能
TEST(CellTest, SequenceNumber) {
    sl::Cell<int, true> cell(42);
    EXPECT_EQ(cell.seq_.load(), 42);
    
    // 测试序列号更新
    cell.seq_.store(43);
    EXPECT_EQ(cell.seq_.load(), 43);
}

// 测试 Cell 的内存对齐
TEST(CellTest, MemoryAlignment) {
    static_assert(alignof(sl::Cell<int, true>) >= sl::cache_line, "Cell should be cache line aligned");
    static_assert(alignof(sl::Cell<std::string, true>) >= sl::cache_line, "Cell should be cache line aligned");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 