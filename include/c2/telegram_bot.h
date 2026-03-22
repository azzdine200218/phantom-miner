#ifndef XMR_TELEGRAM_BOT_H
#define XMR_TELEGRAM_BOT_H

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <map>


    namespace XMR {

  class TelegramBot {
  public:
    TelegramBot(const std::string &token, const std::string &chat_id);
    ~TelegramBot();

    void start();
    void stop();
    void send_message(const std::string &text);
    void send_stats(double hashrate, uint64_t accepted, uint64_t rejected,
                    double uptime);

    using CommandCallback = std::function<void(const std::string &command)>;
    void set_command_handler(CommandCallback cb);

  private:
    void poll_loop();
    std::string http_get(const std::string &url);
    std::string http_post(const std::string &url, const std::string &data);
    void send_request(const std::string &method, const std::string &params);

    std::string m_token;
    std::string m_chat_id;
    std::atomic<bool> m_running;
    std::thread m_thread;
    CommandCallback m_command_handler;
    int m_last_update_id;
  };

} // namespace XMR

#endif
