#include "../../include/network/pool_balancer.h"
#include <algorithm>
#include <atomic>
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

PoolBalancer::PoolBalancer() = default;

void PoolBalancer::add_pool(const std::string &url, int priority) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_pools.push_back({url, priority, 0.0, true});
  sort_pools();
}

PoolBalancer::Pool PoolBalancer::get_best_pool() {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &p : m_pools) {
    if (p.available)
      return p;
  }
  return m_pools.empty() ? Pool{"", 0, 0, false} : m_pools[0];
}

void PoolBalancer::update_latency(const std::string &url, double ms) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &p : m_pools) {
    if (p.url == url) {
      p.latency = ms;
      break;
    }
  }
  sort_pools();
}

void PoolBalancer::set_availability(const std::string &url, bool available) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &p : m_pools) {
    if (p.url == url) {
      p.available = available;
      break;
    }
  }
}

void PoolBalancer::sort_pools() {
  std::sort(m_pools.begin(), m_pools.end(), [](const Pool &a, const Pool &b) {
    if (a.priority != b.priority)
      return a.priority < b.priority;
    return a.latency < b.latency;
  });
}

} // namespace XMR
