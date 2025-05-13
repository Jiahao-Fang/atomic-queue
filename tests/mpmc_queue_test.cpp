#include <gtest/gtest.h>
#include "../include/atomic_queue.hpp"
#include <thread>
#include <vector>
#include <windows.h>
#include <string>
#include <pthread.h>
#include <sched.h>
// Test basic functionality of MPMCQueue
TEST(MPMCQueueTest, BasicOperations) {
    sl::MPMCQueue<int, 4> queue;
    int value;
    
    // Test basic push and pop operations
    EXPECT_TRUE(queue.try_push(1));
    EXPECT_TRUE(queue.try_push(2));
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 2);
}

// Test queue full/empty states
TEST(MPMCQueueTest, FullEmptyQueue) {
    sl::MPMCQueue<int, 2> queue;
    int value;
    
    // Test empty queue
    EXPECT_FALSE(queue.try_pop(value));
    
    // Fill the queue
    EXPECT_TRUE(queue.try_push(1));
    EXPECT_TRUE(queue.try_push(2));
    EXPECT_FALSE(queue.try_push(3)); // Queue is full
}

// Test multi-threading operations
TEST(MPMCQueueTest, MultiThreading) {
    const int QUEUE_SIZE = 1024;
    const int NUM_PRODUCERS = 4;
    const int NUM_CONSUMERS = 4;
    const int ITEMS_PER_PRODUCER = 100;
    
    sl::MPMCQueue<int, QUEUE_SIZE> queue;
    std::atomic<int> sum(0);
    
    // Producer thread
    auto producer = [&](int id) {
        // Set thread affinity to ensure one thread per CPU core
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
    
    // Consumer thread
    auto consumer = [&](int id) {
        // Set thread affinity to ensure one thread per CPU core
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
    
    // Start producers
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        producers.emplace_back(producer, i);
    }
    
    // Start consumers
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        consumers.emplace_back(consumer, i);
    }
    // Wait for all threads to complete
    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();
    std::cout << "sum: " << sum.load() << std::endl;
    EXPECT_EQ(sum.load(), NUM_PRODUCERS * ITEMS_PER_PRODUCER);
}

// Test different types
TEST(MPMCQueueTest, DifferentTypes) {
    // Test simple types
    {
        sl::MPMCQueue<double, 4> queue;
        double value;
        EXPECT_TRUE(queue.try_push(1.5));
        EXPECT_TRUE(queue.try_pop(value));
        EXPECT_DOUBLE_EQ(value, 1.5);
    }
    // Test complex types
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

// Test emplace functionality
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

// Test memory alignment
TEST(MPMCQueueTest, MemoryAlignment) {
    static_assert(alignof(sl::MPMCQueue<int, 4>) >= sl::cache_line,
                 "MPMCQueue should be cache line aligned");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 