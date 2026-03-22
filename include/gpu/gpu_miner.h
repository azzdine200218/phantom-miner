#ifndef XMR_GPU_MINER_H
#define XMR_GPU_MINER_H

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>


#ifdef __OPENCL__
#include <CL/cl.h>
#endif

#ifdef __CUDA__
#include <cuda_runtime.h>
#include <string>
#include <functional>
#include <stdexcept>
#include <map>

#endif

namespace XMR {

enum GPUPlatform { GPU_CUDA, GPU_OPENCL };

class GPUMiner {
public:
    GPUMiner(GPUPlatform platform, int device_id, int threads_per_block = 256);
    ~GPUMiner();

    bool initialize();
    bool is_available() const {
        return m_initialized;
    }

    // Hash functions
    uint32_t find_nonce(const std::vector<uint8_t>& blob, uint64_t target);
    void     hash_batch(const std::vector<std::vector<uint8_t>>& inputs, std::vector<std::vector<uint8_t>>& outputs);

    // Mining functions
    void mine_loop(const std::vector<uint8_t>& blob, uint64_t target, std::atomic<bool>& running,
                   std::atomic<uint32_t>& nonce);

    // Performance monitoring
    uint64_t total_hashes() const {
        return m_total_hashes;
    }
    double get_hashrate() const;
    size_t get_memory_usage() const;
    int    get_temperature() const;

    // Device management
    bool                     select_device(int device_id);
    std::vector<std::string> get_available_devices();
    std::string              get_device_name() const;

    // Optimization
    void optimize_for_algorithm(const std::string& algorithm);
    void set_intensity(int intensity);
    void set_worksize(int worksize);

private:
    // OpenCL implementation
#ifdef __OPENCL__
    bool             init_opencl();
    void             cleanup_opencl();
    cl_context       m_cl_context;
    cl_command_queue m_cl_queue;
    cl_program       m_cl_program;
    cl_kernel        m_cl_kernel;
    cl_mem           m_cl_input_buffer;
    cl_mem           m_cl_output_buffer;
#endif

    // CUDA implementation
#ifdef __CUDA__
    bool         init_cuda();
    void         cleanup_cuda();
    cudaStream_t m_cuda_stream;
    void*        m_cuda_input_buffer;
    void*        m_cuda_output_buffer;
    void*        m_cuda_randomx_vm;
#endif

    GPUPlatform           m_platform;
    int                   m_device_id;
    int                   m_threads_per_block;
    std::atomic<bool>     m_initialized;
    std::atomic<uint64_t> m_total_hashes;

    // Performance tracking
    std::atomic<uint64_t> m_total_time_ns;
    std::atomic<double>   m_current_hashrate;

    // Mining parameters
    int m_intensity;
    int m_worksize;

    // Thread management
    std::vector<std::thread> m_mining_threads;
    std::mutex               m_mutex;

    // Device info
    std::string m_device_name;
    size_t      m_device_memory;
    int         m_compute_units;
};

// Advanced GPU mining manager
class GPUManager {
public:
    static GPUManager& instance();

    void initialize_all_gpus();
    void cleanup_all_gpus();

    GPUMiner*              get_best_gpu();
    std::vector<GPUMiner*> get_all_gpus();

    void distribute_work(const std::vector<uint8_t>& blob, uint64_t target);
    void collect_results(std::vector<uint32_t>& nonces);

    double get_total_hashrate() const;
    size_t get_total_memory_usage() const;

private:
    std::vector<std::unique_ptr<GPUMiner>> m_gpus;
    std::mutex                             m_mutex;
};

}  // namespace XMR

#endif