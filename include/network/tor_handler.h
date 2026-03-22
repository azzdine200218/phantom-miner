#ifndef XMR_TOR_HANDLER_H
#define XMR_TOR_HANDLER_H

#include <string>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <functional>
#include <stdexcept>
#include <map>


    namespace XMR {

  class TorHandler {
  public:
    TorHandler(const std::string &socks5_host = "127.0.0.1",
               int socks5_port = 9050);
    bool start_tor_process();
    bool stop_tor_process();
    bool is_tor_running();
    bool create_tor_socket(int &socket_fd);
    void set_new_identity();

  private:
    std::string m_host;
    int m_port;
    int m_tor_pid;
    bool socks5_handshake(int fd);
    bool socks5_connect(int fd, const std::string &host, int port);
  };

} // namespace XMR

#endif
