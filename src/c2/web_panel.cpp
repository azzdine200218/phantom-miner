#include "../../include/c2/web_panel.h"
#include <curl/curl.h>
#include <iostream>
#include <nlohmann/json.hpp>
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

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            std::string *output) {
  size_t total = size * nmemb;
  output->append(static_cast<char *>(contents), total);
  return total;
}

WebPanel::WebPanel(const std::string &url, const std::string &api_key)
    : m_url(url), m_api_key(api_key) {}

void WebPanel::send_stats(double hashrate, uint64_t accepted,
                          uint64_t rejected) {
  nlohmann::json data;
  data["hashrate"] = hashrate;
  data["accepted"] = accepted;
  data["rejected"] = rejected;
  data["api_key"] = m_api_key;

  CURL *curl = curl_easy_init();
  if (!curl)
    return;

  std::string response;
  curl_easy_setopt(curl, CURLOPT_URL, (m_url + "/api/stats").c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.dump().c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
}

} // namespace XMR
