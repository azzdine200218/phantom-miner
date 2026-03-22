#include "../../include/stealth/environment_detector.h"

#include <cstring>
#include <fstream>
#include <string>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

#ifdef __linux__
#include <sys/ptrace.h>

#endif

namespace Stealth {

bool EnvironmentDetector::is_running_in_vm() {
  return check_vm_cpu() || check_vm_mac() || check_vm_processes();
}

bool EnvironmentDetector::is_running_in_debugger() {
#ifdef __linux__
  if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1)
    return true;
  std::ifstream status("/proc/self/status");
  std::string line;
  while (std::getline(status, line)) {
    if (line.find("TracerPid:") != std::string::npos) {
      int pid = std::stoi(line.substr(line.find(":") + 1));
      if (pid > 0)
        return true;
    }
  }
#endif
  return false;
}

bool EnvironmentDetector::is_running_in_sandbox() {
  return check_sandbox_files() || check_sandbox_network();
}

bool EnvironmentDetector::is_network_monitored() {
  return (access("/usr/bin/wireshark", F_OK) == 0 ||
          access("/usr/bin/tcpdump", F_OK) == 0);
}

bool EnvironmentDetector::should_exit_if_sandbox() {
  if (is_running_in_vm() || is_running_in_debugger() || is_running_in_sandbox())
    return true;
  return false;
}

bool EnvironmentDetector::check_vm_cpu() {
  std::ifstream cpuinfo("/proc/cpuinfo");
  std::string line;
  while (std::getline(cpuinfo, line)) {
    if (line.find("hypervisor") != std::string::npos ||
        line.find("KVM") != std::string::npos ||
        line.find("VMware") != std::string::npos)
      return true;
  }
  return false;
}

bool EnvironmentDetector::check_vm_mac() {
  // Simplified; would read /sys/class/net/eth0/address
  return false;
}

bool EnvironmentDetector::check_vm_processes() {
  const char *procs[] = {"vboxservice", "vmtoolsd", "VBoxService", nullptr};
  for (int i = 0; procs[i]; ++i) {
    std::string cmd = "pgrep " + std::string(procs[i]) + " > /dev/null 2>&1";
    if (system(cmd.c_str()) == 0)
      return true;
  }
  return false;
}

bool EnvironmentDetector::check_debugger_linux() {
#ifdef __linux__
  return (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1);
#else
  return false;
#endif
}

bool EnvironmentDetector::check_sandbox_files() {
  const char *files[] = {"/.dockerenv", "/.dockerinit", "/proc/1/cgroup",
                         nullptr};
  for (int i = 0; files[i]; ++i) {
    struct stat st;
    if (stat(files[i], &st) == 0)
      return true;
  }
  return false;
}

bool EnvironmentDetector::check_sandbox_network() {
  std::ifstream resolv("/etc/resolv.conf");
  std::string line;
  while (std::getline(resolv, line)) {
    if (line.find("nameserver 127.0.0.11") != std::string::npos)
      return true;
  }
  return false;
}

} // namespace Stealth
