#include "../include/atomic_queue.hpp"
#include <thread>
#include <vector>
#include <iostream>
#include <chrono>

// Example demonstrating the usage of SPMC (Single Producer Multiple Consumer) queue
int main() {
    // Create an SPMC queue with size 1024
    sl::SPMCQueue<int, 1024> queue;
    
    // Number of consumers
    const int NUM_CONSUMERS = 4;
    const int TOTAL_ITEMS = 10000;
    
    // Atomic counter to track processed items
    std::atomic<int> processed_items(0);
    
    // Producer function (single producer)
    auto producer = [&]() {
        for (int i = 0; i < TOTAL_ITEMS; ++i) {
            // Try to push until successful
            while (!queue.try_push(i)) {
                std::this_thread::yield();
            }
        }
    };
    
    // Consumer function
    auto consumer = [&](int id) {
        // Get a reader for this consumer
        auto reader = queue.getReader();
        
        while (processed_items.load(std::memory_order_relaxed) < TOTAL_ITEMS) {
            // Try to read a value
            if (int* value = reader.read()) {
                processed_items.fetch_add(1, std::memory_order_relaxed);
                // Process the value (in this example, we just print it)
                std::cout << "Consumer " << id << " processed: " << *value << std::endl;
            }
            std::this_thread::yield();
        }
    };
    
    // Create and start producer thread
    std::thread producer_thread(producer);
    
    // Create and start consumer threads
    std::vector<std::thread> consumers;
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back(consumer, i);
    }
    
    // Wait for producer to finish
    producer_thread.join();
    
    // Wait for all consumers to finish
    for (auto& c : consumers) {
        c.join();
    }
    
    // Verify all items were processed
    std::cout << "Total processed items: " << processed_items.load() << std::endl;
    std::cout << "Expected items: " << TOTAL_ITEMS << std::endl;
    
    return 0;
} 