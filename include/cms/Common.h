#ifndef CMS_COMMON_H
#define CMS_COMMON_H

#include <string>
#include <memory>
#include <cstdint>

namespace cms {

// Version information
constexpr const char* VERSION = "1.0.0";
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;

// Version string macro for compatibility
#define CMS_VERSION_STRING "1.0.0"

// Common type aliases
using String = std::string;
using byte = uint8_t;

// Platform detection helpers
#if defined(CMS_PLATFORM_WINDOWS)
    constexpr const char* PLATFORM_NAME = "Windows";
#elif defined(CMS_PLATFORM_MACOS)
    constexpr const char* PLATFORM_NAME = "macOS";
#elif defined(CMS_PLATFORM_LINUX)
    constexpr const char* PLATFORM_NAME = "Linux";
#else
    constexpr const char* PLATFORM_NAME = "Unknown";
#endif

// Return codes
enum class StatusCode : int {
    Success = 0,
    Error = 1,
    InvalidArgument = 2,
    NotFound = 3,
    AlreadyExists = 4,
    PermissionDenied = 5,
    NetworkError = 6
};

// Convert StatusCode to string
inline const char* StatusCodeToString(StatusCode code) {
    switch (code) {
        case StatusCode::Success: return "Success";
        case StatusCode::Error: return "Error";
        case StatusCode::InvalidArgument: return "Invalid Argument";
        case StatusCode::NotFound: return "Not Found";
        case StatusCode::AlreadyExists: return "Already Exists";
        case StatusCode::PermissionDenied: return "Permission Denied";
        case StatusCode::NetworkError: return "Network Error";
        default: return "Unknown";
    }
}

} // namespace cms

#endif // CMS_COMMON_H
