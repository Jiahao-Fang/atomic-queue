#include <cstddef>
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <functional> 
#include <atomic>
#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

// 设置线程亲和性的跨平台函数
inline void SetThreadAffinity(int cpu_id) {
    #ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 1ULL << cpu_id);
    #elif defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    #endif
}

#include "../include/atomic_queue.hpp"

constexpr uint64_t total_rounds = 100'000'000;
constexpr size_t capacity = 1048576;

using sl_queue_pow2 = typename sl::MPMCQueue<uint64_t, capacity, true, sl::EnablePowerOfTwo>;
//using sl_queue_run_pow2 = typename sl::MPMCQueue<uint64_t, 0>;
using sl_queue_no_pow2 = typename sl::MPMCQueue<uint64_t, capacity + 1, true, sl::DisablePowerOfTwo>;
//using sl_queue_run_no_pow2 = typename sl::MPMCQueue<uint64_t, 0>;
using sl_spmc_queue_pow2 = typename sl::SPMCQueue<uint64_t, capacity, true, sl::EnablePowerOfTwo>;
using sl_spmc_queue_no_pow2 = typename sl::SPMCQueue<uint64_t, capacity + 1, true, sl::DisablePowerOfTwo>;

struct QueueType {
    std::string name;
    std::function<uint64_t(int, size_t)> benchmark;
    std::size_t capacity;
};

template <typename T>
void mpmc_read_worker(T &queue, uint64_t rounds) {
    uint64_t round = 0;
    uint64_t val = 0;

    while (round++ < rounds) {
        queue.try_pop(val);
    }
}

template <typename T>
void mpmc_write_worker(T &queue, uint64_t rounds) {
    uint64_t round = 0;

    while (round++ < rounds) {
        queue.try_push(round);
    }
}

template <typename T>
void spmc_read_worker(typename T::Reader &reader, uint64_t rounds, std::atomic<uint64_t>& read_count) {
    uint64_t local_read_count = 0;
    uint64_t round = 0;
    //rounds = 100000000;
    while (round++ < rounds) {
        if (reader.read() != nullptr) {
            local_read_count++;
        }
    }
    
    read_count.fetch_add(local_read_count, std::memory_order_relaxed);
}

template <typename T>
void spmc_write_worker(T &queue, uint64_t rounds) {
    uint64_t round = 0;

    while (round++ < rounds) {
        if (round % 10) {
            // 执行一些复杂的操作
            std::this_thread::sleep_for(std::chrono::microseconds(1000));

            for (int i = 0; i < 100; ++i) {}
            
        }
        queue.push(round);
    }
}

template <typename T>
auto bounded_mpmc_queue_bench(int num_threads, size_t size) {
    T queue(size);

    std::vector<std::thread> workers(num_threads * 2);

    const auto begin_time = std::chrono::system_clock::now();

    for (int i = 0; i < num_threads; i++) {
        new (&workers[i]) std::thread([&queue, num_threads]() {
            mpmc_write_worker(queue, total_rounds / num_threads);
        });
    }

    for (int i = 0; i < num_threads; i++) {
        new (&workers[num_threads + i]) std::thread([&queue, num_threads]() {
            mpmc_read_worker(queue, total_rounds / num_threads);
        });
    }

    for (int i = 0; i < num_threads * 2; i++) {
        workers[i].join();
    }

    const auto end_time = std::chrono::system_clock::now();

    return (end_time - begin_time).count();
}

template <typename T>
auto bounded_spmc_queue_bench(int num_readers, size_t size) {
    T queue(size);

    std::vector<std::thread> workers(num_readers + 1);
    std::vector<typename T::Reader> readers(num_readers);
    std::vector<std::atomic<uint64_t>> read_counts(num_readers);
    
    for (auto& count : read_counts) {
        count.store(0, std::memory_order_relaxed);
    }

    const auto begin_time = std::chrono::system_clock::now();

    // 创建一个写线程
    new (&workers[0]) std::thread([&queue, num_readers]() {
        spmc_write_worker(queue, total_rounds);
    });

    // 创建多个读线程
    for (int i = 0; i < num_readers; i++) {
        readers[i] = queue.getReader();
        new (&workers[i + 1]) std::thread([&readers, &read_counts, i, num_readers]() {
            // 设置线程亲和性
            SetThreadAffinity(i % std::thread::hardware_concurrency());
            
            spmc_read_worker<T>(readers[i], total_rounds, read_counts[i]);
        });
    }

    for (int i = 0; i < num_readers + 1; i++) {
        workers[i].join();
    }

    const auto end_time = std::chrono::system_clock::now();
    
    // Output statistics for each reader
    std::cout << "Reader statistics:" << std::endl;
    uint64_t total_read = 0;
    for (int i = 0; i < num_readers; i++) {
        uint64_t count = read_counts[i].load(std::memory_order_relaxed);
        total_read += count;
        std::cout << "  Reader " << i << " read " << count << " items" << std::endl;
    }
    std::cout << "  Total read: " << total_read << " items" << std::endl;

    return (end_time - begin_time).count();
}

void run_bench(int num_threads, const std::vector<QueueType>& queue_types) {
    fprintf(stdout, "=== Producers=%d - Consumers=%d ===\n", num_threads, num_threads);

    const int time_width = 15;
    const int label_width = 15;

    for (const auto& queue_type : queue_types) {
        auto time_us = queue_type.benchmark(num_threads, queue_type.capacity) / 1000;
        std::cout << std::setw(label_width) << std::left << queue_type.name
                  << std::setw(time_width) << std::right << time_us << "us | Total rounds = " << total_rounds << std::endl;
    }
}

void run_spmc_bench(int num_readers, const std::vector<QueueType>& queue_types) {
    fprintf(stdout, "=== SPMC: Producer=1 - Consumers=%d ===\n", num_readers);

    const int time_width = 15;
    const int label_width = 15;

    for (const auto& queue_type : queue_types) {
        auto time_us = queue_type.benchmark(num_readers, queue_type.capacity) / 1000;
        std::cout << std::setw(label_width) << std::left << queue_type.name
                  << std::setw(time_width) << std::right << time_us << "us | Total rounds = " << total_rounds << std::endl;
    }
}

int main(int argc, char *argv[]) {
    // std::vector<QueueType> mpmc_queue_types = {
    //     {"sl-pow2", bounded_mpmc_queue_bench<sl_queue_pow2>, capacity},
    //     //{"sl-run-pow2", bounded_mpmc_queue_bench<sl_queue_run_pow2>, capacity},
    //     {"sl-no-pow2", bounded_mpmc_queue_bench<sl_queue_no_pow2>, capacity + 1},
    //     //{"sl-run-no-pow2", bounded_mpmc_queue_bench<sl_queue_run_no_pow2>, capacity + 1}
    // };
    
    std::vector<QueueType> spmc_queue_types = {
        {"sl-spmc-pow2", bounded_spmc_queue_bench<sl_spmc_queue_pow2>, capacity},
        {"sl-spmc-no-pow2", bounded_spmc_queue_bench<sl_spmc_queue_no_pow2>, capacity + 1},
    };

    for (int i = 1; i < argc; i++) {
        int thread_count = std::atoi(argv[i]);
        // run_bench(thread_count, mpmc_queue_types);
        run_spmc_bench(thread_count, spmc_queue_types);
    }

    return 0;
}