#ifndef XMR_RANDOMX_ENGINE_H
#define XMR_RANDOMX_ENGINE_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <randomx.h>
#include <thread>
#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
#include <map>


namespace XMR {

    class RandomXEngine {
    public:
        enum Mode { MODE_FAST = 0, MODE_LIGHT = 1 };

        // Enhanced constructor with NUMA awareness and memory optimization
        RandomXEngine(bool huge_pages = true, Mode mode = MODE_FAST, bool avx512 = false, bool numa_aware = true,
                      size_t cache_size_mb = 2080);
        ~RandomXEngine();

        RandomXEngine(const RandomXEngine&)            = delete;
        RandomXEngine& operator=(const RandomXEngine&) = delete;

        // Optimized hash functions with pre-allocated buffers
        std::vector<uint8_t> hash(const std::vector<uint8_t>& input);
        std::vector<uint8_t> hash(const uint8_t* data, size_t size);

        // High-performance hash with output buffer reuse
        void hash(const uint8_t* data, size_t size, uint8_t* output);

        // Batch hashing for improved throughput
        void hash_batch(const std::vector<std::vector<uint8_t>>& inputs, std::vector<std::vector<uint8_t>>& outputs);

        void reseed(const std::vector<uint8_t>& seed);
        bool is_valid() const {
            return m_initialized;
        }

        uint64_t total_hashes() const {
            return m_total_hashes;
        }
        double average_time_ms() const;

        // Performance monitoring
        double current_hashrate() const;
        size_t memory_usage() const;

        // Memory optimization
        void prefetch_dataset();
        void optimize_memory_layout();
        
        // Advanced optimization functions
        void hash_advanced(const uint8_t* data, size_t size, uint8_t* output);
        void prefetch_data(const uint8_t* data, size_t size);
        void optimize_for_cpu();

    private:
        void initialize();
        void cleanup();
        void setup_huge_pages();
        void setup_numa();
        void allocate_optimized_memory();
        void detect_cpu_features();
        void optimize_cache_line_usage();

        // Memory pool for reduced allocation overhead
        class MemoryPool;
        std::unique_ptr<MemoryPool> m_memory_pool;

        bool   m_huge_pages;
        Mode   m_mode;
        bool   m_avx512;
        bool   m_numa_aware;
        size_t m_cache_size_mb;

        randomx_cache* m_cache;
        randomx_vm*    m_vm;

        std::atomic<bool>     m_initialized;
        std::atomic<uint64_t> m_total_hashes;
        std::atomic<uint64_t> m_total_time_ns;

        // Performance tracking
        std::atomic<uint64_t> m_last_hash_time;
        std::atomic<double>   m_current_hashrate;

        // Thread safety
        mutable std::mutex m_mutex;

        // Pre-allocated buffers for performance
        std::vector<uint8_t> m_hash_buffer;

        // Dataset prefetching
        bool m_dataset_prefetched;

        // Advanced optimizations
        bool m_use_sse41 = false;
        bool m_use_avx2 = false;
        bool m_use_avx512 = false;
        bool m_use_bmi2 = false;

        // Cache optimization
        size_t m_cache_line_size = 64;
        bool   m_cache_optimized = false;

        // Thread-local storage for better performance
        thread_local static std::vector<uint8_t> t_hash_buffer;
        thread_local static std::vector<uint8_t> t_blob_buffer;
    };

}  // namespace XMR

#endif
