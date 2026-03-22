#ifndef XMR_POOL_BALANCER_H
#define XMR_POOL_BALANCER_H

#include <mutex>
#include <string>
#include <vector>
#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <functional>
#include <stdexcept>
#include <map>


    namespace XMR {

  class PoolBalancer {
  public:
    struct Pool {
      std::string url;
      int priority;
      double latency;
      bool available;
    };

    PoolBalancer();
    void add_pool(const std::string &url, int priority = 1);
    Pool get_best_pool();
    void update_latency(const std::string &url, double ms);
    void set_availability(const std::string &url, bool available);

  private:
    std::vector<Pool> m_pools;
    std::mutex m_mutex;
    void sort_pools();
  };

} // namespace XMR

#endif
