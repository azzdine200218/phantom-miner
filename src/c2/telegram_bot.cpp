#include "../../include/c2/telegram_bot.h"
#include <chrono>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
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

TelegramBot::TelegramBot(const std::string &token, const std::string &chat_id)
    : m_token(token), m_chat_id(chat_id), m_running(false),
      m_last_update_id(0) {}

TelegramBot::~TelegramBot() { stop(); }

void TelegramBot::start() {
  if (m_running)
    return;
  m_running = true;
  m_thread = std::thread(&TelegramBot::poll_loop, this);
}

void TelegramBot::stop() {
  m_running = false;
  if (m_thread.joinable())
    m_thread.join();
}

void TelegramBot::send_message(const std::string &text) {
  nlohmann::json params;
  params["chat_id"] = m_chat_id;
  params["text"] = text;
  params["parse_mode"] = "Markdown";
  send_request("sendMessage", params.dump());
}

void TelegramBot::send_stats(double hashrate, uint64_t accepted,
                             uint64_t rejected, double uptime) {
  std::string msg = "📊 *Miner Stats*\n";
  msg += "Hashrate: `" + std::to_string(static_cast<int>(hashrate)) + " H/s`\n";
  msg += "Shares: " + std::to_string(accepted) + "/" +
         std::to_string(accepted + rejected) + "\n";
  msg += "Uptime: " + std::to_string(static_cast<int>(uptime)) + " seconds";
  send_message(msg);
}

void TelegramBot::set_command_handler(CommandCallback cb) {
  m_command_handler = cb;
}

void TelegramBot::poll_loop() {
  while (m_running) {
    std::string url = "https://api.telegram.org/bot" + m_token + "/getUpdates";
    if (m_last_update_id > 0)
      url += "?offset=" + std::to_string(m_last_update_id + 1);
    std::string response = http_get(url);
    if (!response.empty()) {
      try {
        auto json = nlohmann::json::parse(response);
        if (json.contains("result") && json["result"].is_array()) {
          for (auto &update : json["result"]) {
            int update_id = update["update_id"];
            if (update_id > m_last_update_id)
              m_last_update_id = update_id;
            if (update.contains("message") &&
                update["message"].contains("text")) {
              std::string text = update["message"]["text"];
              if (m_command_handler)
                m_command_handler(text);
            }
          }
        }
      } catch (...) {
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
}

std::string TelegramBot::http_get(const std::string &url) {
  CURL *curl = curl_easy_init();
  if (!curl)
    return "";
  std::string response;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  return (res == CURLE_OK) ? response : "";
}

std::string TelegramBot::http_post(const std::string &url,
                                   const std::string &data) {
  CURL *curl = curl_easy_init();
  if (!curl)
    return "";
  std::string response;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  return (res == CURLE_OK) ? response : "";
}

void TelegramBot::send_request(const std::string &method,
                               const std::string &params) {
  std::string url = "https://api.telegram.org/bot" + m_token + "/" + method;
  http_post(url, params);
}

} // namespace XMR
