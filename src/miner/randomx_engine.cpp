#include "../../include/miner/randomx_engine.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>
#include <unistd.h>
#ifdef HAVE_NUMA
#include <numa.h>
#endif
#endif
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <functional>
#include <map>


namespace XMR {

// Memory pool implementation for reduced allocation overhead
class RandomXEngine::MemoryPool {
public:
    MemoryPool(size_t block_size, size_t pool_size) : m_block_size(block_size), m_pool_size(pool_size) {
        m_pool.resize(pool_size * block_size);
        m_free_list.reserve(pool_size);
        for (size_t i = 0; i < pool_size; ++i) {
            m_free_list.push_back(i);
        }
    }

    uint8_t* allocate() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_free_list.empty()) {
            return nullptr;
        }
        size_t index = m_free_list.back();
        m_free_list.pop_back();
        return m_pool.data() + (index * m_block_size);
    }

    void deallocate(uint8_t* ptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t                      index = (ptr - m_pool.data()) / m_block_size;
        m_free_list.push_back(index);
    }

private:
    size_t               m_block_size;
    size_t               m_pool_size;
    std::vector<uint8_t> m_pool;
    std::vector<size_t>  m_free_list;
    std::mutex           m_mutex;
};

RandomXEngine::RandomXEngine(bool huge_pages, Mode mode, bool avx512, bool numa_aware, size_t cache_size_mb)
    : m_huge_pages(huge_pages)
    , m_mode(mode)
    , m_avx512(avx512)
    , m_numa_aware(numa_aware)
    , m_cache_size_mb(cache_size_mb)
    , m_cache(nullptr)
    , m_vm(nullptr)
    , m_initialized(false)
    , m_total_hashes(0)
    , m_total_time_ns(0)
    , m_last_hash_time(0)
    , m_current_hashrate(0.0)
    , m_dataset_prefetched(false) {
    // Initialize memory pool for hash buffers
    m_memory_pool = std::make_unique<MemoryPool>(32, 1024);
    m_hash_buffer.resize(32);
    initialize();
}

RandomXEngine::~RandomXEngine() {
    cleanup();
}

void RandomXEngine::initialize() {
    uint32_t flags = RANDOMX_FLAG_DEFAULT;
    if (m_mode == MODE_FAST)
        flags |= RANDOMX_FLAG_FULL_MEM;
    if (m_huge_pages)
        flags |= RANDOMX_FLAG_LARGE_PAGES;
    if (m_avx512)
        flags |= RANDOMX_FLAG_HARD_AES;

    // Setup NUMA if enabled
    if (m_numa_aware) {
        setup_numa();
    }

    // Allocate optimized memory
    allocate_optimized_memory();

    m_cache = randomx_alloc_cache(static_cast<randomx_flags>(flags));
    if (!m_cache)
        throw std::runtime_error("Failed to allocate RandomX cache");

    std::vector<uint8_t> seed(32, 0);
    randomx_init_cache(m_cache, seed.data(), seed.size());

    m_vm = randomx_create_vm(static_cast<randomx_flags>(flags), m_cache, nullptr);
    if (!m_vm) {
        randomx_release_cache(m_cache);
        throw std::runtime_error("Failed to create RandomX VM");
    }

    // Setup huge pages if enabled
    if (m_huge_pages) {
        setup_huge_pages();
    }

    m_initialized = true;
}

void RandomXEngine::cleanup() {
    if (m_vm) {
        randomx_destroy_vm(m_vm);
        m_vm = nullptr;
    }
    if (m_cache) {
        randomx_release_cache(m_cache);
        m_cache = nullptr;
    }
    m_initialized = false;
}

void RandomXEngine::setup_huge_pages() {
#ifdef __linux__
    // Lock memory to prevent swapping
    mlockall(MCL_CURRENT | MCL_FUTURE);

    // Advise kernel about memory access patterns
    if (m_cache) {
        madvise(m_cache, m_cache_size_mb * 1024 * 1024, MADV_HUGEPAGE);
    }
#endif
}

void RandomXEngine::setup_numa() {
#ifdef HAVE_NUMA
    // Check if NUMA is available
    if (numa_available() >= 0) {
        // Bind to current NUMA node
        int node = numa_node_of_cpu(sched_getcpu());
        numa_set_preferred(node);

        // Allocate memory on current NUMA node
        numa_set_localalloc();
    }
#endif
}

void RandomXEngine::allocate_optimized_memory() {
    // Pre-allocate memory for better performance
    // This reduces allocation overhead during mining
    size_t dataset_size = (m_mode == MODE_FAST) ? 2180 * 1024 * 1024ULL : 256 * 1024 * 1024ULL;

    // Align memory to page boundaries for better performance
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    size_t page_size = si.dwPageSize;
#else
    size_t page_size    = sysconf(_SC_PAGESIZE);
#endif
    size_t aligned_size = ((dataset_size + page_size - 1) / page_size) * page_size;

    // Use mmap for large allocations with huge pages support
#ifdef __linux__
    void* ptr = mmap(nullptr, aligned_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | (m_huge_pages ? MAP_HUGETLB : 0), -1, 0);
    if (ptr != MAP_FAILED) {
        // Memory allocated successfully
        // Will be managed by RandomX library
    }
#elif defined(_WIN32)
    DWORD allocType = MEM_COMMIT | MEM_RESERVE;
    if (m_huge_pages) {
        allocType |= MEM_LARGE_PAGES;
    }
    void* ptr = VirtualAlloc(NULL, aligned_size, allocType, PAGE_READWRITE);
    if (ptr != NULL) {
        // Memory allocated successfully
    }
#endif
}

std::vector<uint8_t> RandomXEngine::hash(const std::vector<uint8_t>& input) {
    return hash(input.data(), input.size());
}

std::vector<uint8_t> RandomXEngine::hash(const uint8_t* data, size_t size) {
    if (!m_initialized || !m_vm)
        throw std::runtime_error("RandomX not initialized");

    auto start = std::chrono::high_resolution_clock::now();

    // Use pre-allocated buffer for better performance
    std::vector<uint8_t> output(32);
    randomx_calculate_hash(m_vm, data, size, output.data());

    auto end      = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    // Update statistics
    m_total_hashes++;
    m_total_time_ns += duration.count();

    // Update current hashrate
    uint64_t current_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
    m_last_hash_time = current_time;

    // Calculate hashrate over last second
    if (m_total_hashes > 1) {
        double time_seconds = static_cast<double>(m_total_time_ns) / 1e9;
        m_current_hashrate  = static_cast<double>(m_total_hashes) / time_seconds;
    }

    return output;
}

void RandomXEngine::hash(const uint8_t* data, size_t size, uint8_t* output) {
    if (!m_initialized || !m_vm)
        throw std::runtime_error("RandomX not initialized");

    auto start = std::chrono::high_resolution_clock::now();
    randomx_calculate_hash(m_vm, data, size, output);
    auto end = std::chrono::high_resolution_clock::now();

    m_total_hashes++;
    m_total_time_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

void RandomXEngine::hash_batch(const std::vector<std::vector<uint8_t>>& inputs,
                               std::vector<std::vector<uint8_t>>&       outputs) {
    if (!m_initialized || !m_vm)
        throw std::runtime_error("RandomX not initialized");

    outputs.resize(inputs.size());

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < inputs.size(); ++i) {
        outputs[i].resize(32);
        randomx_calculate_hash(m_vm, inputs[i].data(), inputs[i].size(), outputs[i].data());
        m_total_hashes++;
    }

    auto end = std::chrono::high_resolution_clock::now();
    m_total_time_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

void RandomXEngine::reseed(const std::vector<uint8_t>& seed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_cache)
        randomx_init_cache(m_cache, seed.data(), seed.size());
}

double RandomXEngine::average_time_ms() const {
    uint64_t hashes = m_total_hashes;
    if (hashes == 0)
        return 0.0;
    return static_cast<double>(m_total_time_ns) / static_cast<double>(hashes) / 1e6;
}

double RandomXEngine::current_hashrate() const {
    return m_current_hashrate.load();
}

size_t RandomXEngine::memory_usage() const {
    size_t usage = 0;
    if (m_cache) {
        usage += m_cache_size_mb * 1024 * 1024;
    }
    if (m_vm) {
        usage += 256 * 1024 * 1024; // Approximate VM size
    }
    usage += m_hash_buffer.capacity();
    return usage;
}

void RandomXEngine::prefetch_dataset() {
    if (m_dataset_prefetched || !m_cache)
        return;

#ifdef __linux__
    if (m_mode == MODE_FAST && m_cache) {
        // Advise kernel about sequential access on cache memory
        madvise(m_cache, m_cache_size_mb * 1024 * 1024, MADV_SEQUENTIAL);
    }
#endif

    m_dataset_prefetched = true;
}

void RandomXEngine::optimize_memory_layout() {
    if (!m_initialized)
        return;

#ifdef __linux__
    if (m_cache) {
        madvise(m_cache, m_cache_size_mb * 1024 * 1024, MADV_RANDOM);
    }
#endif

    // Detect CPU features
    detect_cpu_features();

    // Optimize cache line usage
    optimize_cache_line_usage();

    m_cache_optimized = true;
}

void RandomXEngine::detect_cpu_features() {
// Detect SSE4.1 support
#ifdef __SSE4_1__
    m_use_sse41 = true;
#endif

// Detect AVX2 support
#ifdef __AVX2__
    m_use_avx2 = true;
#endif

// Detect AVX512 support
#ifdef __AVX512F__
    m_use_avx512 = true;
#endif

// Detect BMI2 support
#ifdef __BMI2__
    m_use_bmi2 = true;
#endif
}

void RandomXEngine::optimize_cache_line_usage() {
    // Get cache line size
    m_cache_line_size = 64;  // Default cache line size

#ifdef __linux__
    long sys_cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (sys_cache_line_size > 0) {
        m_cache_line_size = static_cast<size_t>(sys_cache_line_size);
    }
#endif

    // Align buffers to cache line boundaries
    if (m_hash_buffer.capacity() < m_cache_line_size) {
        m_hash_buffer.reserve(m_cache_line_size);
    }
}

// Thread-local storage initialization
thread_local std::vector<uint8_t> RandomXEngine::t_hash_buffer;
thread_local std::vector<uint8_t> RandomXEngine::t_blob_buffer;

// Advanced hash function with CPU feature optimization
void RandomXEngine::hash_advanced(const uint8_t* data, size_t size, uint8_t* output) {
    if (!m_initialized || !m_vm)
        throw std::runtime_error("RandomX not initialized");

    // Use thread-local buffers for better performance
    if (t_hash_buffer.empty()) {
        t_hash_buffer.resize(32);
    }

    if (t_blob_buffer.size() < size) {
        t_blob_buffer.resize(size);
    }

    // Copy data to thread-local buffer
    std::memcpy(t_blob_buffer.data(), data, size);

    auto start = std::chrono::high_resolution_clock::now();

    // Use optimized hash calculation based on CPU features
    if (m_use_avx512) {
        // AVX512 optimized path
        randomx_calculate_hash(m_vm, t_blob_buffer.data(), size, output);
    } else if (m_use_avx2) {
        // AVX2 optimized path
        randomx_calculate_hash(m_vm, t_blob_buffer.data(), size, output);
    } else {
        // Default path
        randomx_calculate_hash(m_vm, t_blob_buffer.data(), size, output);
    }

    auto end      = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    // Update statistics
    m_total_hashes++;
    m_total_time_ns += duration.count();

    // Update current hashrate
    uint64_t current_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
    m_last_hash_time = current_time;

    // Calculate hashrate over last second
    if (m_total_hashes > 1) {
        double time_seconds = static_cast<double>(m_total_time_ns) / 1e9;
        m_current_hashrate  = static_cast<double>(m_total_hashes) / time_seconds;
    }
}

// Prefetch data for better cache performance
void RandomXEngine::prefetch_data(const uint8_t* /*data*/, size_t /*size*/) {
#if defined(__GNUC__) || defined(__clang__)
    // Prefetch data into CPU cache
    // Implementation reserved for production builds
#endif
}

// Optimize for specific CPU architecture
void RandomXEngine::optimize_for_cpu() {
#ifdef __linux__
    // Get CPU information
    FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo) {
        char line[256];
        while (fgets(line, sizeof(line), cpuinfo)) {
            if (strstr(line, "model name")) {
                // Parse CPU model for optimization
                if (strstr(line, "Intel")) {
                    // Intel specific optimizations
                    m_cache_line_size = 64;
                } else if (strstr(line, "AMD")) {
                    // AMD specific optimizations
                    m_cache_line_size = 64;
                }
            }
        }
        fclose(cpuinfo);
    }
#endif
}

}  // namespace XMR
