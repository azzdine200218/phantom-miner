#ifndef XMR_STATS_H
#define XMR_STATS_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <functional>
#include <stdexcept>
#include <map>


namespace XMR {

struct MiningStats {
    std::atomic<uint64_t> total_hashes{0};
    std::atomic<uint64_t> accepted_shares{0};
    std::atomic<uint64_t> rejected_shares{0};
    std::atomic<double> current_hashrate{0.0};
    std::chrono::steady_clock::time_point start_time;

    MiningStats() : start_time(std::chrono::steady_clock::now()) {}

    MiningStats(const MiningStats& other) {
        total_hashes.store(other.total_hashes.load());
        accepted_shares.store(other.accepted_shares.load());
        rejected_shares.store(other.rejected_shares.load());
        current_hashrate.store(other.current_hashrate.load());
        start_time = other.start_time;
    }

    double uptime_seconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - start_time).count();
    }

    void reset() {
        total_hashes = 0;
        accepted_shares = 0;
        rejected_shares = 0;
        current_hashrate = 0.0;
        start_time = std::chrono::steady_clock::now();
    }
};

} // namespace XMR

#endif
