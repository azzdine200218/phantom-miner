#include "../../include/miner/miner.h"

#include "../../include/network/pool_client.h"
#include "../../include/utils/logger.h"
#include <openssl/sha.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <immintrin.h>
#include <iomanip>
#include <random>
#include <sstream>
#include <thread>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <map>



namespace XMR {

class MiningManager::Worker {
public:
    Worker(int id, const Config& cfg) : m_id(id), m_config(cfg), m_running(false), m_paused(false), m_nonce_counter(0) {
        if (!cfg.network.pools.empty()) {
            std::string url = cfg.network.pools[0].at("url");
            m_client        = std::make_unique<PoolClient>(url);
        }
        if (cfg.mining.randomx.enabled) {
            m_rx = std::make_unique<RandomXEngine>(cfg.mining.randomx.huge_pages, RandomXEngine::MODE_FAST,
                                                   cfg.mining.randomx.avx512,
                                                   true,  // NUMA aware
                                                   cfg.mining.randomx.cache_size_mb);
        }

        // Pre-allocate buffers for better performance
        m_blob_buffer.reserve(256);
        m_hash_buffer.resize(32);
    }

    void start() {
        m_running          = true;
        m_paused           = false;
        m_stats.start_time = std::chrono::steady_clock::now();
        m_nonce_counter    = 0;
    }

    void stop() {
        m_running = false;
        if (m_client)
            m_client->disconnect();
    }

    void pause() {
        m_paused = true;
    }
    void resume() {
        m_paused = false;
    }
    bool is_running() const {
        return m_running;
    }
    MiningStats get_stats() const {
        return m_stats;
    }

    void run() {
        if (!m_running)
            return;

        while (m_running) {
            if (m_paused) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            try {
                if (!m_client->is_connected()) {
                    if (!m_client->connect()) {
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                        continue;
                    }
                    if (!m_client->login(m_config.network.wallet, m_config.network.password,
                                         m_config.network.user_agent)) {
                        m_client->disconnect();
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                        continue;
                    }
                }

                while (m_running && !m_paused && m_client->is_connected()) {
                    std::string response = m_client->receive_line();
                    if (response.empty())
                        break;

                    if (response.find("job") != std::string::npos) {
                        size_t json_start = response.find('{');
                        if (json_start != std::string::npos) {
                            auto json = nlohmann::json::parse(response.substr(json_start));
                            if (json.contains("params")) {
                                auto        params = json["params"];
                                std::string blob   = params["blob"];
                                std::string target = params["target"];
                                std::string job_id = params["job_id"];

                                // Optimized blob parsing
                                std::vector<uint8_t> blob_bytes;
                                blob_bytes.reserve(blob.length() / 2);
                                for (size_t i = 0; i < blob.length(); i += 2) {
                                    blob_bytes.push_back(std::stoi(blob.substr(i, 2), nullptr, 16));
                                }

                                uint64_t target_int = std::stoull(target, nullptr, 16);

                                // Use optimized nonce finding
                                uint32_t nonce = find_nonce_optimized(blob_bytes, target_int);

                                if (nonce != 0xFFFFFFFF) {
                                    std::stringstream ss;
                                    ss << "{\"method\":\"submit\",\"params\":{"
                                       << "\"id\":\"" << job_id << "\","
                                       << "\"job_id\":\"" << job_id << "\","
                                       << "\"nonce\":\"" << std::hex << std::setw(8) << std::setfill('0') << nonce
                                       << "\","
                                       << "\"result\":\"" << blob << "\""
                                       << "},\"id\":2}\n";
                                    if (m_client->send(ss.str())) {
                                        std::string reply = m_client->receive_line();
                                        if (reply.find("accept") != std::string::npos ||
                                            reply.find("success") != std::string::npos)
                                            m_stats.accepted_shares++;
                                        else
                                            m_stats.rejected_shares++;
                                    }
                                }
                                m_stats.total_hashes += m_hashes_per_job;
                            }
                        }
                    }
                }
            } catch (...) {
                m_client->disconnect();
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }

private:
    // Optimized nonce finding with better performance
    uint32_t find_nonce_optimized(const std::vector<uint8_t>& blob, uint64_t target) {
        // Use sequential nonce search for better cache performance
        uint32_t start_nonce = m_nonce_counter.fetch_add(m_hashes_per_job);

        // Pre-allocate blob copy to avoid repeated allocations
        if (m_blob_buffer.size() < blob.size()) {
            m_blob_buffer.resize(blob.size());
        }
        std::copy(blob.begin(), blob.end(), m_blob_buffer.begin());

        // Ensure we have enough space for nonce
        if (m_blob_buffer.size() < 43) {
            m_blob_buffer.resize(43);
        }

        for (uint32_t i = 0; i < m_hashes_per_job; ++i) {
            uint32_t nonce = start_nonce + i;

            // Set nonce in little-endian format
            m_blob_buffer[39] = nonce & 0xFF;
            m_blob_buffer[40] = (nonce >> 8) & 0xFF;
            m_blob_buffer[41] = (nonce >> 16) & 0xFF;
            m_blob_buffer[42] = (nonce >> 24) & 0xFF;

            // Calculate hash
            std::vector<uint8_t> hash;
            if (m_rx) {
                // Use RandomX engine with optimized hash function
                m_rx->hash(m_blob_buffer.data(), m_blob_buffer.size(), m_hash_buffer.data());
                hash = m_hash_buffer;
            } else {
                hash = sha256(m_blob_buffer);
            }

            // Check if hash meets target
            uint64_t hash_int = 0;
            for (int j = 0; j < 8; ++j) {
                hash_int |= (uint64_t)hash[j] << (j * 8);
            }

            if (hash_int < target) {
                return nonce;
            }
        }

        return 0xFFFFFFFF;
    }

    // Fallback SHA256 implementation
    std::vector<uint8_t> sha256(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> hash(32);
        SHA256(data.data(), data.size(), hash.data());
        return hash;
    }

    int                            m_id;
    Config                         m_config;
    std::atomic<bool>              m_running;
    std::atomic<bool>              m_paused;
    std::atomic<uint32_t>          m_nonce_counter;
    std::unique_ptr<PoolClient>    m_client;
    std::unique_ptr<RandomXEngine> m_rx;
    MiningStats                    m_stats;

    // Performance optimization members
    static constexpr uint32_t m_hashes_per_job = 10000;
    std::vector<uint8_t>      m_blob_buffer;
    std::vector<uint8_t>      m_hash_buffer;
};

MiningManager::MiningManager(const Config& cfg) : m_config(cfg), m_global_running(false) {
    int num_threads = cfg.mining.max_threads;
    if (num_threads <= 0) {
        num_threads = std::thread::hardware_concurrency() - cfg.mining.reserved_cores;
        if (num_threads <= 0)
            num_threads = 1;
    }

    // Limit threads based on CPU usage setting
    if (cfg.mining.max_cpu_usage > 0 && cfg.mining.max_cpu_usage < 100) {
        int max_threads = (num_threads * cfg.mining.max_cpu_usage) / 100;
        if (max_threads < num_threads) {
            num_threads = max_threads;
        }
    }

    for (int i = 0; i < num_threads; ++i) {
        m_workers.push_back(std::make_unique<Worker>(i, cfg));
    }
}

MiningManager::~MiningManager() {
    stop_all();
}

void MiningManager::start_all() {
    m_global_running = true;
    for (auto& w : m_workers) {
        w->start();
        m_threads.emplace_back(&MiningManager::worker_loop, this, w.get());
    }
}

void MiningManager::stop_all() {
    m_global_running = false;
    for (auto& w : m_workers)
        w->stop();
    for (auto& t : m_threads)
        if (t.joinable())
            t.join();
    m_threads.clear();
}

void MiningManager::pause_all() {
    for (auto& w : m_workers)
        w->pause();
}

void MiningManager::resume_all() {
    for (auto& w : m_workers)
        w->resume();
}

MiningStats MiningManager::get_total_stats() const {
    MiningStats total;
    for (auto& w : m_workers) {
        auto s = w->get_stats();
        total.total_hashes = total.total_hashes + s.total_hashes;
        total.accepted_shares = total.accepted_shares + s.accepted_shares;
        total.rejected_shares = total.rejected_shares + s.rejected_shares;
        total.current_hashrate = total.current_hashrate + s.current_hashrate;
    }
    return total;
}

void MiningManager::update_config(const nlohmann::json& new_cfg) {
    m_config = Config::from_json(new_cfg);
}

void MiningManager::worker_loop(Worker* worker) {
    worker->run();
}

}  // namespace XMR
