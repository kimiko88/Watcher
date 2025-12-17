#include "cms/Common.h"
#include "cms/Logger.h"
#include "cms/Config.h"
#include <iostream>

// Forward declarations from Core.cpp
namespace cms {
namespace core {
    bool Initialize();
    void Shutdown();
    String GetSystemInfo();
}
}

int main(int argc, char* argv[]) {
    using namespace cms;
    
    // Set log level to Debug for client
    Logger::Instance().SetLogLevel(LogLevel::Debug);
    
    LOG_INFO("=== CMS Client Starting ===");
    LOG_INFO(String("Platform: ") + PLATFORM_NAME);
    LOG_INFO(String("Version: ") + VERSION);
    
    // Initialize core
    if (!core::Initialize()) {
        LOG_ERROR("Failed to initialize CMS Core");
        return static_cast<int>(StatusCode::Error);
    }
    
    // Configure the system
    Config::Instance().Set("app.name", "CMS Client");
    Config::Instance().Set("app.type", "client");
    
    LOG_INFO(String("System Info: ") + core::GetSystemInfo());
    LOG_INFO(String("Config - App Name: ") + Config::Instance().GetOr("app.name", "N/A"));
    
    std::cout << "\nCMS Client is running. Press Enter to exit..." << std::endl;
    std::cin.get();
    
    // Shutdown
    core::Shutdown();
    LOG_INFO("=== CMS Client Stopped ===");
    
    return static_cast<int>(StatusCode::Success);
}
