#ifndef XMR_REMOTE_CONFIG_H
#define XMR_REMOTE_CONFIG_H

#include <atomic>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <map>


    namespace XMR {

  class RemoteConfigClient {
  public:
    RemoteConfigClient(const std::string &url, const std::string &token,
                 int update_interval_sec);
    ~RemoteConfigClient();

    void start();
    void stop();

    using ConfigCallback = std::function<void(const nlohmann::json &)>;
    void set_on_update(ConfigCallback cb);

  private:
    void fetch_loop();
    nlohmann::json fetch_config();
    bool verify_signature(const nlohmann::json &config);

    std::string m_url;
    std::string m_token;
    int m_interval;
    std::atomic<bool> m_running;
    std::thread m_thread;
    ConfigCallback m_callback;
  };

} // namespace XMR

#endif
