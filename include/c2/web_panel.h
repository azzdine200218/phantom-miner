#ifndef XMR_WEB_PANEL_H
#define XMR_WEB_PANEL_H

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

  class WebPanel {
  public:
    WebPanel(const std::string &url, const std::string &api_key);
    void send_stats(double hashrate, uint64_t accepted, uint64_t rejected);

  private:
    std::string m_url;
    std::string m_api_key;
  };

} // namespace XMR

#endif
