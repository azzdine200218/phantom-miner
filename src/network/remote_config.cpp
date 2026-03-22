#include "../../include/network/remote_config.h"
#include <chrono>
#include <curl/curl.h>
#include <thread>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <map>



namespace XMR {

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            std::string *output) {
  size_t total = size * nmemb;
  output->append(static_cast<char *>(contents), total);
  return total;
}

RemoteConfigClient::RemoteConfigClient(const std::string &url, const std::string &token,
                           int update_interval_sec)
    : m_url(url), m_token(token), m_interval(update_interval_sec),
      m_running(false) {}

RemoteConfigClient::~RemoteConfigClient() { stop(); }

void RemoteConfigClient::start() {
  if (m_running)
    return;
  m_running = true;
  m_thread = std::thread(&RemoteConfigClient::fetch_loop, this);
}

void RemoteConfigClient::stop() {
  m_running = false;
  if (m_thread.joinable())
    m_thread.join();
}

void RemoteConfigClient::set_on_update(ConfigCallback cb) { m_callback = cb; }

void RemoteConfigClient::fetch_loop() {
  while (m_running) {
    try {
      nlohmann::json config = fetch_config();
      if (!config.empty() && verify_signature(config)) {
        if (m_callback)
          m_callback(config);
      }
    } catch (...) {
    }
    std::this_thread::sleep_for(std::chrono::seconds(m_interval));
  }
}

nlohmann::json RemoteConfigClient::fetch_config() {
  CURL *curl = curl_easy_init();
  if (!curl)
    return {};

  std::string response;
  curl_easy_setopt(curl, CURLOPT_URL, m_url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

  struct curl_slist *headers = nullptr;
  if (!m_token.empty()) {
    std::string auth = "Authorization: Bearer " + m_token;
    headers = curl_slist_append(headers, auth.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  }

  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK)
    return {};

  try {
    return nlohmann::json::parse(response);
  } catch (...) {
    return {};
  }
}

bool RemoteConfigClient::verify_signature(const nlohmann::json &config) {
  // For demonstration, always true
  return true;
}

} // namespace XMR
