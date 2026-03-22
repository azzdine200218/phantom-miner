#ifndef XMR_POOL_CLIENT_H
#define XMR_POOL_CLIENT_H

#include <atomic>
#include <mutex>
#include <string>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>
#include <functional>
#include <stdexcept>
#include <map>


    namespace XMR {

  class PoolClient {
  public:
    explicit PoolClient(const std::string &pool_url);
    ~PoolClient();

    bool connect();
    void disconnect();
    bool is_connected() const { return m_connected; }

    bool send(const std::string &data);
    bool send_line(const std::string &data);
    std::string receive();
    std::string receive_line();

    bool login(const std::string &wallet, const std::string &password,
               const std::string &agent);

  private:
    bool parse_url(const std::string &url);
    bool create_socket();
    void close_socket();

    std::string m_host;
    int m_port;
    bool m_use_ssl;
    int m_socket;
    void *m_ssl; // SSL* placeholder
    std::atomic<bool> m_connected;
    std::mutex m_mutex;
    int m_timeout;
  };

} // namespace XMR

#endif
