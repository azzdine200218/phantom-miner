#ifndef XMR_LOGGER_H
#define XMR_LOGGER_H

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

  class Logger {
  public:
    static void init(const std::string &log_file);
    static void info(const std::string &msg);
    static void warning(const std::string &msg);
    static void error(const std::string &msg);
    static void debug(const std::string &msg);
    static void
    set_level(int level); // 0=off, 1=error, 2=warning, 3=info, 4=debug

  private:
    static void log(const std::string &level, const std::string &msg);
    static std::string m_log_file;
    static int m_level;
  };

} // namespace XMR

#endif
