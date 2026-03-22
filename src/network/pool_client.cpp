#include "../../include/network/pool_client.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <cstring>
#include <iostream>


#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

namespace XMR {

PoolClient::PoolClient(const std::string &pool_url)
    : m_socket(-1), m_ssl(nullptr), m_connected(false), m_timeout(30) {
  parse_url(pool_url);
}

PoolClient::~PoolClient() { disconnect(); }

bool PoolClient::parse_url(const std::string &url) {
  std::string temp = url;
  size_t proto = temp.find("://");
  if (proto != std::string::npos) {
    if (temp.substr(0, proto).find("ssl") != std::string::npos ||
        temp.substr(0, proto).find("tls") != std::string::npos)
      m_use_ssl = true;
    temp = temp.substr(proto + 3);
  }
  size_t colon = temp.find(":");
  if (colon != std::string::npos) {
    m_host = temp.substr(0, colon);
    m_port = std::stoi(temp.substr(colon + 1));
  } else {
    m_host = temp;
    m_port = m_use_ssl ? 443 : 3333;
  }
  return true;
}

bool PoolClient::create_socket() {
#ifdef _WIN32
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
  m_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_socket < 0)
    return false;

#ifdef _WIN32
  DWORD timeoutMs = m_timeout * 1000;
  setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
  setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
#else
#ifdef _WIN32
  DWORD timeoutMs = m_timeout * 1000;
  setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
  setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
#else
  struct timeval tv;
  tv.tv_sec = m_timeout;
  tv.tv_usec = 0;
  setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
#endif

  struct hostent *server = gethostbyname(m_host.c_str());
  if (!server)
    return false;

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
  addr.sin_port = htons(m_port);

  if (::connect(m_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close_socket();
    return false;
  }
  return true;
}

void PoolClient::close_socket() {
  if (m_socket >= 0) {
#ifdef _WIN32
    closesocket(m_socket);
#else
    close(m_socket);
#endif
    m_socket = -1;
  }
  if (m_ssl) {
    // SSL cleanup placeholder
    m_ssl = nullptr;
  }
}

bool PoolClient::connect() {
  if (m_connected)
    return true;
  if (!create_socket())
    return false;
  // SSL handshake would go here if m_use_ssl
  m_connected = true;
  return true;
}

void PoolClient::disconnect() {
  close_socket();
  m_connected = false;
}

bool PoolClient::send(const std::string &data) {
  if (!m_connected)
    return false;
  std::lock_guard<std::mutex> lock(m_mutex);
  int sent = ::send(m_socket, data.c_str(), data.length(), 0);
  return sent == static_cast<int>(data.length());
}

bool PoolClient::send_line(const std::string &data) {
  return send(data + "\n");
}

std::string PoolClient::receive() {
  if (!m_connected)
    return "";
  std::lock_guard<std::mutex> lock(m_mutex);
  char buffer[4096];
  int n = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
  if (n > 0) {
    buffer[n] = '\0';
    return std::string(buffer);
  }
  return "";
}

std::string PoolClient::receive_line() {
  if (!m_connected)
    return "";
  std::lock_guard<std::mutex> lock(m_mutex);
  std::string line;
  char ch;
  while (true) {
    int n = recv(m_socket, &ch, 1, 0);
    if (n <= 0)
      return "";
    if (ch == '\n')
      break;
    if (ch != '\r')
      line += ch;
  }
  return line;
}

bool PoolClient::login(const std::string &wallet, const std::string &password,
                       const std::string &agent) {
  std::string login_msg = "{\"method\":\"login\",\"params\":{"
                          "\"login\":\"" +
                          wallet +
                          "\","
                          "\"pass\":\"" +
                          password +
                          "\","
                          "\"agent\":\"" +
                          agent + "\"},\"id\":1}\n";
  if (!send(login_msg))
    return false;
  std::string resp = receive_line();
  return resp.find("error") == std::string::npos;
}

} // namespace XMR
