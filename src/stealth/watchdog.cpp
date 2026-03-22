#include "../../include/stealth/watchdog.h"
#include <chrono>
#include <cstring>
#ifndef _WIN32
#include <dirent.h>
#include <signal.h>
#else
#include <windows.h>
#include <tlhelp32.h>
#endif
#include <fstream>
#include <thread>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <map>



namespace Stealth {

Watchdog::Watchdog() : m_running(false), m_check_interval(5) {}

Watchdog::~Watchdog() { stop(); }

void Watchdog::start() {
  if (m_running)
    return;
  m_running = true;
  m_thread = std::thread(&Watchdog::monitor_loop, this);
}

void Watchdog::stop() {
  m_running = false;
  if (m_thread.joinable())
    m_thread.join();
}

void Watchdog::add_blacklist(const std::string &process_name) {
  m_blacklist.push_back(process_name);
}

void Watchdog::remove_blacklist(const std::string &process_name) {
  m_blacklist.erase(
      std::remove(m_blacklist.begin(), m_blacklist.end(), process_name),
      m_blacklist.end());
}

void Watchdog::set_check_interval(int seconds) { m_check_interval = seconds; }

void Watchdog::set_on_detection(std::function<void()> callback) {
  m_on_detection = callback;
}

void Watchdog::monitor_loop() {
  while (m_running) {
    for (const auto &name : m_blacklist) {
      if (is_process_running(name)) {
        kill_process(name);
        if (m_on_detection)
          m_on_detection();
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(m_check_interval));
  }
}

bool Watchdog::is_process_running(const std::string &name) {
#ifdef __linux__
  DIR *dir = opendir("/proc");
  if (!dir)
    return false;
  struct dirent *entry;
  bool found = false;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
      std::string comm_path = "/proc/" + std::string(entry->d_name) + "/comm";
      std::ifstream f(comm_path);
      std::string comm;
      if (std::getline(f, comm) && comm == name) {
        found = true;
        break;
      }
    }
  }
  closedir(dir);
  return found;
#else
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnapshot == INVALID_HANDLE_VALUE) return false;
  PROCESSENTRY32 pe32;
  pe32.dwSize = sizeof(PROCESSENTRY32);
  bool found = false;
  if (Process32First(hSnapshot, &pe32)) {
    do {
      if (std::string(pe32.szExeFile) == name) {
        found = true;
        break;
      }
    } while (Process32Next(hSnapshot, &pe32));
  }
  CloseHandle(hSnapshot);
  return found;
#endif
}

bool Watchdog::kill_process(const std::string &name) {
#ifdef __linux__
  DIR *dir = opendir("/proc");
  if (!dir)
    return false;
  struct dirent *entry;
  bool killed = false;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
      std::string comm_path = "/proc/" + std::string(entry->d_name) + "/comm";
      std::ifstream f(comm_path);
      std::string comm;
      if (std::getline(f, comm) && comm == name) {
        pid_t pid = std::stoi(entry->d_name);
        kill(pid, SIGKILL);
        killed = true;
        break;
      }
    }
  }
  closedir(dir);
  return killed;
#else
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnapshot == INVALID_HANDLE_VALUE) return false;
  PROCESSENTRY32 pe32;
  pe32.dwSize = sizeof(PROCESSENTRY32);
  bool killed = false;
  if (Process32First(hSnapshot, &pe32)) {
    do {
      if (std::string(pe32.szExeFile) == name) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
        if (hProcess != NULL) {
            TerminateProcess(hProcess, 1);
            CloseHandle(hProcess);
            killed = true;
        }
      }
    } while (Process32Next(hSnapshot, &pe32));
  }
  CloseHandle(hSnapshot);
  return killed;
#endif
}

} // namespace Stealth
