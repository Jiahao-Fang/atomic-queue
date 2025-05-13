# Atomic Queue Implementation in C++20

## Overview

This project implements high-performance atomic queues for concurrent programming, inspired by [Fedor Pikus's CppCon talk](https://www.youtube.com/watch?v=8uAW5FQtcvE) and real-world experience in high-frequency trading environments. The repository provides two main queue implementations:

1. **MPMC (Multiple Producer Multiple Consumer)** - Designed for request-response patterns where multiple threads can both enqueue and dequeue items.
2. **SPMC (Single Producer Multiple Consumer)** - Optimized for market data distribution scenarios where a single thread produces data that needs to be consumed by multiple processing threads.

## Key Features

- Leverages C++20 features for improved performance and cleaner code
- Uses compile-time techniques to reduce branching and improve latency
- Provides specialized queue implementations for different concurrency patterns
- Optimized for low-latency environments like high-frequency trading systems
- Utilizes C++20 concepts to support different parameter configurations and metadata types

## Implementation Details

The implementation uses several optimization techniques:
- Compile-time configuration with concepts and constraints allowing for flexible queue customization
- Support for different element types and metadata through template parameters and concepts
- Cache-line alignment to reduce false sharing
- Power-of-two size optimization for modulo operations
- Specialized buffer implementations for different use cases
- Configurable size constraints and buffer types through template parameters

## Performance

The queues are designed with performance in mind, focusing on minimizing latency which is critical in trading applications. Benchmark results demonstrate the efficiency of these implementations compared to traditional approaches.

## Usage

### Basic Usage Examples

#### MPMC Queue Example
```cpp
#include <atomic_queue.hpp>
#include <thread>
#include <vector>

int main() {
    // Create an MPMC queue with size 1024
    sl::MPMCQueue<int, 1024> queue;
    
    // Producer thread
    auto producer = [&queue]() {
        for(int i = 0; i < 1000; ++i) {
            queue.push(i);  // or use queue.emplace(i)
        }
    };
    
    // Consumer thread
    auto consumer = [&queue]() {
        int value;
        while(queue.try_pop(value)) {
            // Process value
        }
    };
    
    // Create multiple producer and consumer threads
    std::vector<std::thread> producers(4);
    std::vector<std::thread> consumers(4);
    
    for(auto& p : producers) p = std::thread(producer);
    for(auto& c : consumers) c = std::thread(consumer);
    
    for(auto& p : producers) p.join();
    for(auto& c : consumers) c.join();
}
```

#### SPMC Queue Example
```cpp
#include <atomic_queue.hpp>
#include <thread>
#include <vector>

int main() {
    // Create an SPMC queue with size 1024
    sl::SPMCQueue<int, 1024> queue;
    
    // Producer thread
    auto producer = [&queue]() {
        for(int i = 0; i < 1000; ++i) {
            queue.push(i);  // or use queue.emplace(i)
        }
    };
    
    // Consumer thread
    auto consumer = [&queue]() {
        auto reader = queue.getReader();
        while(true) {
            if(int* value = reader.read()) {
                // Process value
            }
        }
    };
    
    // Create single producer thread
    std::thread producer_thread(producer);
    
    // Create multiple consumer threads
    std::vector<std::thread> consumers(4);
    for(auto& c : consumers) c = std::thread(consumer);
    
    producer_thread.join();
    for(auto& c : consumers) c.join();
}
```

### Template Parameters

Both queue implementations support the following template parameters:

```cpp
template<
    typename T,           // Element type
    size_t N,            // Queue size
    bool Modulo = true,  // Whether to use modulo operation
    IsValidConstraint SizeConstraint = EnablePowerOfTwo,  // Size constraint
    IsValidBufferType BufferType = UseHeapBuffer         // Buffer type
>
```

### Key Methods

#### MPMC Queue Methods
- `push(const T& value)` - Push a value into the queue
- `emplace(Args&&... args)` - Construct a new element in the queue
- `try_push(const T& value)` - Try to push a value, returns false if queue is full
- `try_emplace(Args&&... args)` - Try to construct a new element
- `pop(T& value)` - Pop a value from the queue
- `try_pop(T& value)` - Try to pop a value from the queue

#### SPMC Queue Methods
- `push(const T& value)` - Push a value into the queue
- `emplace(Args&&... args)` - Construct a new element in the queue
- `getReader()` - Get a reader instance
- `Reader::read()` - Read the next value through the reader

### Performance Considerations

1. For MPMC Queue:
   - Suitable for multiple producer and consumer scenarios
   - Use `try_push` and `try_pop` to avoid blocking
   - Queue size must be a power of two (when using EnablePowerOfTwo)

2. For SPMC Queue:
   - Suitable for single producer multiple consumer scenarios
   - Each consumer needs an independent Reader
   - Consumers may read duplicate data
   - Slower consumers may miss data

## Implementation References

### MPMC Queue
The MPMC (Multiple Producer Multiple Consumer) queue implementation is based on the [bounded MPMC queue algorithm](https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue) by Dmitry Vyukov. This lock-free algorithm provides efficient concurrent access for multiple producers and consumers with bounded capacity.

### SPMC Queue
The SPMC (Single Producer Multiple Consumer) queue implementation draws inspiration from [MengRao's SPMC_Queue](https://github.com/MengRao/SPMC_Queue/tree/master). This implementation is specifically optimized for scenarios with a single writer and multiple readers.

### Important Note on SPMC Queue Behavior
It's important to understand that the SPMC queue implementation has a fundamental characteristic: multiple readers operate independently and may read the same data elements. Additionally, there's a possibility that slower readers might miss data if the writer overwrites older elements before they are consumed. This behavior is visible in the performance tests.

In the SPMC design:
- Each reader maintains its own read position
- Readers do not coordinate with each other
- The writer can potentially overwrite data that hasn't been read by slower readers
- There is no guarantee that all readers will process all data elements

While this behavior is rarely problematic in production environments (where readers typically keep up with the writer), it's a critical design consideration when implementing systems that require guaranteed delivery to all consumers.

## Benchmark Results

This section presents the performance test results for MPMC and SPMC queues, comparing them with other popular atomic queue implementations.

### Testing Methodology

To ensure fair comparison, we used the following testing approach:
- Simple int64 as the queue data type
- Blocking push and pop operations
- Each test processes 100 million operations (total_rounds = 100,000,000)
- Queue capacity set to 1,048,576 elements
- All tests run on identical hardware and system environments
- Same test workload for all queue implementations
- Each test represents the average of 10 runs

### Test Environment

Tests were conducted on an AWS EC2 instance with the following specifications:
- Architecture: x86_64
- CPU: Intel(R) Xeon(R) Platinum 8223CL CPU @ 3.00GHz
- Cores: 8 physical cores, 16 logical cores (2 threads per core)
- CPU Features: AVX, AVX2, AVX512
- Memory: 16GB RAM
- Operating System: Amazon Linux 2

### Compared Implementations

We compared our implementation with the following high-star atomic queue implementations from GitHub:

1. [max0x7ba/atomic_queue](https://github.com/max0x7ba/atomic_queue)
2. [rigtorp/MPMCQueue](https://github.com/rigtorp/MPMCQueue)
3. [erez-strauss/lockfree_mpmc_queue](https://github.com/erez-strauss/lockfree_mpmc_queue)

### MPMC Queue Performance Results

The following table shows the performance comparison of MPMC queues with different thread counts (lower values indicate better performance):

| Queue Implementation | 1 Producer/1 Consumer | 2 Producers/2 Consumers | 4 Producers/4 Consumers | 8 Producers/8 Consumers |
|----------------------|----------------------|-------------------------|-------------------------|-------------------------|
| sl-pow2              | 833ms                | 7602ms                  | 7833ms                  | 5635ms                  |
| sl-no-pow2           | 963ms                | 7943ms                  | 7789ms                  | 5655ms                  |
| max0x7ba/atomic_queue| 3507ms               | 8556ms                  | 7944ms                  | 6699ms                  |
| rigtorp/MPMCQueue    | 2293ms               | 9140ms                  | 7905ms                  | 5889ms                  |
| erez-strauss/lockfree| 3677ms               | 13110ms                 | 19776ms                 | 33886ms                 |

### SPMC Queue Performance Results

SPMC queue performance tests demonstrate behavior in single producer, multiple consumer scenarios:

| Queue Implementation | 1 Producer/2 Consumers | 1 Producer/4 Consumers | 1 Producer/8 Consumers |
|----------------------|------------------------|------------------------|------------------------|
| sl-spmc-pow2         | 426ms                  | 482ms                  | 453ms                  |

### Performance Analysis

From the test results, we can observe:

1. **MPMC Queue**:
   - PowerOfTwo-enabled implementations generally perform better than non-PowerOfTwo implementations
   - Performance differences become more pronounced as thread count increases
   - Our implementation performs competitively compared to other popular implementations
   - The erez-strauss implementation shows significant performance degradation with higher thread counts

2. **SPMC Queue**:
   - Our implementation maintains consistent performance regardless of consumer count
   - SPMC queue performance is significantly faster than MPMC implementations

Note that these performance results may vary depending on hardware configuration, operating system, and compiler optimizations. When selecting a queue implementation for a specific application scenario, it's recommended to conduct targeted performance tests.
