#include "cms/Common.h"
#include "cms/Logger.h"
#include "cms/Config.h"

namespace cms {
namespace core {

// Core initialization function
bool Initialize() {
    LOG_INFO("Initializing CMS Core...");
    LOG_INFO(String("Platform: ") + PLATFORM_NAME);
    LOG_INFO(String("Version: ") + VERSION);
    return true;
}

// Core shutdown function
void Shutdown() {
    LOG_INFO("Shutting down CMS Core...");
}

// Example core function
String GetSystemInfo() {
    std::ostringstream oss;
    oss << "Classroom Control System v" << VERSION << " (" << PLATFORM_NAME << ")";
    return oss.str();
}

} // namespace core
} // namespace cms
