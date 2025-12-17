#include "cms/Logger.h"
#include <fstream>
#include <iostream>

namespace cms {

Logger &Logger::getInstance() {
  static Logger instance;
  return instance;
}

void Logger::init(const std::string &logFilePath, bool enableConsole) {
  std::lock_guard<std::mutex> lock(mutex_);
  logFilePath_ = logFilePath;
  enableConsole_ = enableConsole;

  if (!logFilePath_.empty()) {
    fileStream_.open(logFilePath_, std::ios::out | std::ios::app);
    if (!fileStream_.is_open()) {
      std::cerr << "[ERROR] Logger failed to open file: " << logFilePath_
                << std::endl;
    }
  }
}

void Logger::SetLogLevel(LogLevel level) {
  std::lock_guard<std::mutex> lock(mutex_);
  minLevel_ = level;
}

LogLevel Logger::GetLogLevel() const {
  return minLevel_; // Atomic read acceptable or lock if strict
}

void Logger::Log(LogLevel level, const std::string &message) {
  if (level < minLevel_)
    return;

  // Format timestamp could go here
  std::string levelStr = LogLevelToString(level);

  std::lock_guard<std::mutex> lock(mutex_);

  // Console output
  if (enableConsole_) {
    std::cout << "[" << levelStr << "] " << message << std::endl;
  }

  // File output
  if (fileStream_.is_open()) {
    fileStream_ << "[" << levelStr << "] " << message << std::endl;
    fileStream_.flush(); // Ensure written
  }
}

void Logger::Debug(const std::string &message) {
  Log(LogLevel::Debug, message);
}
void Logger::Info(const std::string &message) { Log(LogLevel::Info, message); }
void Logger::Warning(const std::string &message) {
  Log(LogLevel::Warning, message);
}
void Logger::Error(const std::string &message) {
  Log(LogLevel::Error, message);
}

const char *Logger::LogLevelToString(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Warning:
    return "WARNING";
  case LogLevel::Error:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

Logger::~Logger() {
  if (fileStream_.is_open()) {
    fileStream_.close();
  }
}

} // namespace cms
