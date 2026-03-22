cpp
#ifndef XMR_CUDA_MINER_H
#define XMR_CUDA_MINER_H

#include <cstdint>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <functional>
#include <stdexcept>
#include <map>


#ifdef __cplusplus
    extern "C" {
#endif

  bool launch_cuda_miner(const uint8_t *blob, size_t blob_size, uint64_t target,
                         uint32_t *nonce);
  bool init_cuda_device(int device_id);
  void cleanup_cuda();

#ifdef __cplusplus
}
#endif

#endif
