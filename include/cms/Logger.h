#ifndef CMS_LOGGER_H
#define CMS_LOGGER_H

#include "Common.h"
#include <fstream>
#include <mutex>
#include <string>


namespace cms {

// Log severity levels
enum class LogLevel { Debug, Info, Warning, Error };

// Singleton Logger with file and console support
class Logger {
public:
  static Logger &getInstance();

  // Initialize logger configuration
  // Call this once at startup
  void init(const std::string &logFilePath = "", bool enableConsole = true);

  void SetLogLevel(LogLevel level);
  LogLevel GetLogLevel() const;

  void Log(LogLevel level, const std::string &message);

  // Convenience methods
  void Debug(const std::string &message);
  void Info(const std::string &message);
  void Warning(const std::string &message);
  void Error(const std::string &message);

private:
  Logger() : minLevel_(LogLevel::Info), enableConsole_(true) {}
  ~Logger();
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  static const char *LogLevelToString(LogLevel level);

  LogLevel minLevel_;
  bool enableConsole_;
  std::string logFilePath_;
  std::ofstream fileStream_;
  std::mutex mutex_;
};

// Convenience macros using the correct singleton accessor
#define LOG_DEBUG(msg) cms::Logger::getInstance().Debug(msg)
#define LOG_INFO(msg) cms::Logger::getInstance().Info(msg)
#define LOG_WARNING(msg) cms::Logger::getInstance().Warning(msg)
#define LOG_ERROR(msg) cms::Logger::getInstance().Error(msg)

} // namespace cms

#endif // CMS_LOGGER_H
