#include "../include/atomic_queue.hpp"
#include <thread>
#include <vector>
#include <iostream>
#include <chrono>

// Example demonstrating the usage of MPMC (Multiple Producer Multiple Consumer) queue
int main() {
    // Create an MPMC queue with size 1024
    sl::MPMCQueue<int, 1024> queue;
    
    // Number of producers and consumers
    const int NUM_PRODUCERS = 4;
    const int NUM_CONSUMERS = 4;
    const int ITEMS_PER_PRODUCER = 1000;
    
    // Atomic counter to track processed items
    std::atomic<int> processed_items(0);
    
    // Producer function
    auto producer = [&](int id) {
        for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
            int value = id * ITEMS_PER_PRODUCER + i;
            // Try to push until successful
            while (!queue.try_push(value)) {
                std::this_thread::yield();
            }
        }
    };
    
    // Consumer function
    auto consumer = [&](int id) {
        int value;
        while (processed_items.load(std::memory_order_relaxed) < NUM_PRODUCERS * ITEMS_PER_PRODUCER) {
            if (queue.try_pop(value)) {
                processed_items.fetch_add(1, std::memory_order_relaxed);
                // Process the value (in this example, we just print it)
                std::cout << "Consumer " << id << " processed: " << value << std::endl;
            }
            std::this_thread::yield();
        }
    };
    
    // Create and start producer threads
    std::vector<std::thread> producers;
    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(producer, i);
    }
    
    // Create and start consumer threads
    std::vector<std::thread> consumers;
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back(consumer, i);
    }
    
    // Wait for all producers to finish
    for (auto& p : producers) {
        p.join();
    }
    
    // Wait for all consumers to finish
    for (auto& c : consumers) {
        c.join();
    }
    
    // Verify all items were processed
    std::cout << "Total processed items: " << processed_items.load() << std::endl;
    std::cout << "Expected items: " << NUM_PRODUCERS * ITEMS_PER_PRODUCER << std::endl;
    
    return 0;
} 