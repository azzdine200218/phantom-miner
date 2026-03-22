#include "../../include/utils/logger.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>


namespace XMR {

std::string Logger::m_log_file = "";
int Logger::m_level = 3;

void Logger::init(const std::string &log_file) {
  m_log_file = log_file;
  std::ofstream f(log_file, std::ios::trunc);
  f.close();
}

void Logger::info(const std::string &msg) {
  if (m_level >= 3)
    log("INFO", msg);
}

void Logger::warning(const std::string &msg) {
  if (m_level >= 2)
    log("WARN", msg);
}

void Logger::error(const std::string &msg) {
  if (m_level >= 1)
    log("ERROR", msg);
}

void Logger::debug(const std::string &msg) {
  if (m_level >= 4)
    log("DEBUG", msg);
}

void Logger::set_level(int level) { m_level = level; }

void Logger::log(const std::string &level, const std::string &msg) {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::tm tm;
#ifdef _WIN32
  localtime_s(&tm, &time_t);
#else
  localtime_r(&time_t, &tm);
#endif
  std::stringstream ss;
  ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " [" << level << "] " << msg;

  if (!m_log_file.empty()) {
    std::ofstream f(m_log_file, std::ios::app);
    if (f.is_open())
      f << ss.str() << std::endl;
  }
  std::cerr << ss.str() << std::endl;
}

} // namespace XMR
