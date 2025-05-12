#include <gtest/gtest.h>
#include "../include/atomic_queue.hpp"
#include <thread>
#include <vector>
#include <windows.h>
#include <string>
#include <pthread.h>
#include <sched.h>
// 测试 MPMCQueue 的基本功能
TEST(MPMCQueueTest, BasicOperations) {
    sl::MPMCQueue<int, 4> queue;
    int value;
    
    // 测试基本的 push 和 pop
    EXPECT_TRUE(queue.try_push(1));
    EXPECT_TRUE(queue.try_push(2));
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 2);
}

// 测试队列满/空状态
TEST(MPMCQueueTest, FullEmptyQueue) {
    sl::MPMCQueue<int, 2> queue;
    int value;
    
    // 测试空队列
    EXPECT_FALSE(queue.try_pop(value));
    
    // 填满队列
    EXPECT_TRUE(queue.try_push(1));
    EXPECT_TRUE(queue.try_push(2));
    EXPECT_FALSE(queue.try_push(3)); // 队列已满
}

// 测试多线程操作
TEST(MPMCQueueTest, MultiThreading) {
    const int QUEUE_SIZE = 1024;
    const int NUM_PRODUCERS = 4;
    const int NUM_CONSUMERS = 4;
    const int ITEMS_PER_PRODUCER = 100;
    
    sl::MPMCQueue<int, QUEUE_SIZE> queue;
    std::atomic<int> sum(0);
    
    // 生产者线程
    auto producer = [&](int id) {
        // 设置线程亲和性，确保一个线程绑定到一个CPU核心
        HANDLE thread = GetCurrentThread();
        DWORD_PTR mask = 1ULL << (id % std::thread::hardware_concurrency());
        SetThreadAffinityMask(thread, mask);
        
        for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
            int value = id * ITEMS_PER_PRODUCER + i;
            while (!queue.try_push(value)) {
                std::this_thread::yield();
            }
        }
    };
    
    // 消费者线程
    auto consumer = [&](int id) {
        // 设置线程亲和性，确保一个线程绑定到一个CPU核心
        HANDLE thread = GetCurrentThread();
        DWORD_PTR mask = 1ULL << ((NUM_PRODUCERS + id) % std::thread::hardware_concurrency());
        SetThreadAffinityMask(thread, mask);
        int value;
        while (true) {
            if (queue.try_pop(value)) {
                sum.fetch_add(1, std::memory_order_relaxed);
                // std::cout << "consumer " << id << " pop " << sum << std::endl;
            }
            if (sum.load(std::memory_order_relaxed) >= NUM_PRODUCERS * ITEMS_PER_PRODUCER) {
                break;
            }
            std::this_thread::yield();
        }
    };
    
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    
    // 启动生产者
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        producers.emplace_back(producer, i);
    }
    
    // 启动消费者
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        consumers.emplace_back(consumer, i);
    }
    // 等待所有线程完成
    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();
    std::cout << "sum: " << sum.load() << std::endl;
    EXPECT_EQ(sum.load(), NUM_PRODUCERS * ITEMS_PER_PRODUCER);
}

// 测试不同类型
TEST(MPMCQueueTest, DifferentTypes) {
    // 测试简单类型
    {
        sl::MPMCQueue<double, 4> queue;
        double value;
        EXPECT_TRUE(queue.try_push(1.5));
        EXPECT_TRUE(queue.try_pop(value));
        EXPECT_DOUBLE_EQ(value, 1.5);
    }
    // 测试复杂类型
    {
        struct TestStruct {
            int x;
            std::string str;

            TestStruct(int x_, std::string str_) : x(x_), str(std::move(str_)) {}

        };
        
        sl::MPMCQueue<TestStruct, 4> queue;
        TestStruct value(42, "test");
        EXPECT_TRUE(queue.try_push(value));
        TestStruct popped(0, "");
        EXPECT_TRUE(queue.try_pop(popped));
        EXPECT_EQ(popped.x, 42);
        EXPECT_EQ(popped.str, "test");
    }
}   

// 测试 emplace 功能
TEST(MPMCQueueTest, EmplaceTest) {
    struct TestStruct {
        int x;
        std::string str;
        TestStruct(int x_, std::string str_) : x(x_), str(str_) {}
    };
    
    sl::MPMCQueue<TestStruct, 4> queue;
    EXPECT_TRUE(queue.try_emplace(1, "test"));
    std::cout << "test emplace" << std::endl;
    TestStruct value(0, "");
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value.x, 1);
    EXPECT_EQ(value.str, "test");
}

// 测试内存对齐
TEST(MPMCQueueTest, MemoryAlignment) {
    static_assert(alignof(sl::MPMCQueue<int, 4>) >= sl::cache_line,
                 "MPMCQueue should be cache line aligned");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 