#ifndef XMR_MINER_H
#define XMR_MINER_H

#include "../network/pool_client.h"
#include "../utils/config.h"
#include "job.h"
#include "randomx_engine.h"
#include "stats.h"
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <cstdint>
#include <mutex>
#include <string>
#include <functional>
#include <stdexcept>
#include <map>


    namespace XMR {

  class MiningManager {
  public:
    explicit MiningManager(const Config &cfg);
    ~MiningManager();

    void start_all();
    void stop_all();
    void pause_all();
    void resume_all();

    MiningStats get_total_stats() const;
    void update_config(const nlohmann::json &new_cfg);

  private:
    class Worker;
    std::vector<std::unique_ptr<Worker>> m_workers;
    std::vector<std::thread> m_threads;
    Config m_config;
    std::atomic<bool> m_global_running;

    void worker_loop(Worker *worker);
  };

} // namespace XMR

#endif
