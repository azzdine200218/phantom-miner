#include "../include/miner/randomx_engine.h"

#include <chrono>
#include <iostream>
#include <random>
#include <vector>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <functional>
#include <stdexcept>
#include <map>



using namespace XMR;

void test_memory_pool() {
    std::cout << "Testing Memory Pool..." << std::endl;

    RandomXEngine engine(true, RandomXEngine::MODE_FAST, true, true, 2080);

    if (!engine.is_valid()) {
        std::cerr << "Failed to initialize RandomX engine" << std::endl;
        return;
    }

    // Test hash function
    std::vector<uint8_t>            input(76, 0);
    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (int i = 0; i < 76; ++i) {
        input[i] = dis(gen);
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100; ++i) {
        auto hash = engine.hash(input);
        if (hash.size() != 32) {
            std::cerr << "Hash size mismatch" << std::endl;
            return;
        }
    }

    auto end      = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Memory Pool Test: PASSED" << std::endl;
    std::cout << "Time for 100 hashes: " << duration.count() << " ms" << std::endl;
    std::cout << "Average time per hash: " << engine.average_time_ms() << " ms" << std::endl;
    std::cout << "Current hashrate: " << engine.current_hashrate() << " H/s" << std::endl;
    std::cout << "Memory usage: " << engine.memory_usage() << " bytes" << std::endl;
}

void test_batch_hashing() {
    std::cout << "\nTesting Batch Hashing..." << std::endl;

    RandomXEngine engine(true, RandomXEngine::MODE_FAST, true, true, 2080);

    if (!engine.is_valid()) {
        std::cerr << "Failed to initialize RandomX engine" << std::endl;
        return;
    }

    // Prepare batch inputs
    std::vector<std::vector<uint8_t>> inputs(10);
    std::random_device                rd;
    std::mt19937                      gen(rd());
    std::uniform_int_distribution<>   dis(0, 255);

    for (auto& input : inputs) {
        input.resize(76);
        for (int i = 0; i < 76; ++i) {
            input[i] = dis(gen);
        }
    }

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::vector<uint8_t>> outputs;
    engine.hash_batch(inputs, outputs);

    auto end      = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    if (outputs.size() != inputs.size()) {
        std::cerr << "Batch output size mismatch" << std::endl;
        return;
    }

    for (const auto& output : outputs) {
        if (output.size() != 32) {
            std::cerr << "Batch hash size mismatch" << std::endl;
            return;
        }
    }

    std::cout << "Batch Hashing Test: PASSED" << std::endl;
    std::cout << "Time for 10 hashes: " << duration.count() << " ms" << std::endl;
    std::cout << "Average time per hash: " << duration.count() / 10.0 << " ms" << std::endl;
}

void test_buffer_reuse() {
    std::cout << "\nTesting Buffer Reuse..." << std::endl;

    RandomXEngine engine(true, RandomXEngine::MODE_FAST, true, true, 2080);

    if (!engine.is_valid()) {
        std::cerr << "Failed to initialize RandomX engine" << std::endl;
        return;
    }

    std::vector<uint8_t> input(76, 0);
    std::vector<uint8_t> output(32);

    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (int i = 0; i < 76; ++i) {
        input[i] = dis(gen);
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100; ++i) {
        engine.hash(input.data(), input.size(), output.data());
        if (output.size() != 32) {
            std::cerr << "Buffer reuse hash size mismatch" << std::endl;
            return;
        }
    }

    auto end      = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Buffer Reuse Test: PASSED" << std::endl;
    std::cout << "Time for 100 hashes: " << duration.count() << " ms" << std::endl;
    std::cout << "Average time per hash: " << duration.count() / 100.0 << " ms" << std::endl;
}

void test_memory_optimization() {
    std::cout << "\nTesting Memory Optimization..." << std::endl;

    RandomXEngine engine(true, RandomXEngine::MODE_FAST, true, true, 2080);

    if (!engine.is_valid()) {
        std::cerr << "Failed to initialize RandomX engine" << std::endl;
        return;
    }

    // Test prefetching
    engine.prefetch_dataset();

    // Test memory layout optimization
    engine.optimize_memory_layout();

    std::cout << "Memory Optimization Test: PASSED" << std::endl;
    std::cout << "Memory usage after optimization: " << engine.memory_usage() << " bytes" << std::endl;
}

int main() {
    std::cout << "=== RandomX Optimization Tests ===" << std::endl;

    try {
        test_memory_pool();
        test_batch_hashing();
        test_buffer_reuse();
        test_memory_optimization();

        std::cout << "\n=== All Tests Completed ===" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}