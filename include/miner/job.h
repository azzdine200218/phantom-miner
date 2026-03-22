#ifndef XMR_JOB_H
#define XMR_JOB_H

#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <functional>
#include <stdexcept>
#include <map>


namespace XMR {

struct Job {
    std::string job_id;
    std::string blob_hex;
    std::string target_hex;
    uint64_t height;
    std::chrono::steady_clock::time_point received_at;

    Job() : height(0), received_at(std::chrono::steady_clock::now()) {}

    bool is_expired(int timeout_seconds = 30) const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - received_at).count();
        return elapsed > timeout_seconds;
    }
};

} // namespace XMR

#endif
