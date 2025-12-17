#ifndef CMS_LOGGER_H
#define CMS_LOGGER_H

#include "Common.h"
#include <iostream>
#include <sstream>
#include <mutex>

namespace cms {

// Log severity levels
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

// Simple console-based logger
class Logger {
public:
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    void SetLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        minLevel_ = level;
    }

    LogLevel GetLogLevel() const {
        return minLevel_;
    }

    void Log(LogLevel level, const String& message) {
        if (level < minLevel_) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "[" << LogLevelToString(level) << "] " << message << std::endl;
    }

    void Debug(const String& message) {
        Log(LogLevel::Debug, message);
    }

    void Info(const String& message) {
        Log(LogLevel::Info, message);
    }

    void Warning(const String& message) {
        Log(LogLevel::Warning, message);
    }

    void Error(const String& message) {
        Log(LogLevel::Error, message);
    }

private:
    Logger() : minLevel_(LogLevel::Info) {}
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static const char* LogLevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARNING";
            case LogLevel::Error: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    LogLevel minLevel_;
    std::mutex mutex_;
};

// Convenience macros
#define LOG_DEBUG(msg) cms::Logger::Instance().Debug(msg)
#define LOG_INFO(msg) cms::Logger::Instance().Info(msg)
#define LOG_WARNING(msg) cms::Logger::Instance().Warning(msg)
#define LOG_ERROR(msg) cms::Logger::Instance().Error(msg)

} // namespace cms

#endif // CMS_LOGGER_H
