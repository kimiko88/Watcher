#include "cms/Common.h"
#include "cms/Config.h"
#include "cms/Logger.h"
#include <iostream>

// Forward declarations from Core.cpp
namespace cms {
namespace core {
bool Initialize();
void Shutdown();
String GetSystemInfo();
} // namespace core
} // namespace cms

#ifdef _WIN32
#include "cms/platform/WindowsService.h"
#endif

#include <fstream>
void LogMain(const std::string &msg) {
  std::ofstream f("C:\\Users\\Public\\cms_debug.txt", std::ios::app);
  if (f.is_open()) {
    f << "[MAIN] " << msg << std::endl;
    f.close();
  }
}

int main(int argc, char *argv[]) {
  using namespace cms;

  LogMain("Starting main...");
  LogMain("Argc: " + std::to_string(argc));
  for (int i = 0; i < argc; i++) {
    LogMain("Arg[" + std::to_string(i) + "]: " + argv[i]);
  }

  bool runAsService = false;
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--service") {
      runAsService = true;
      break;
    }
  }

  if (runAsService) {
    LogMain("Mode: Service");
  } else {
    LogMain("Mode: Console");
  }

  // Set log level to Debug for client
  Logger::getInstance().SetLogLevel(LogLevel::Debug);

  // Function to run the main application logic
  auto runApplication = []() {
    LOG_INFO("=== CMS Client Starting ===");
    LOG_INFO(String("Platform: ") + PLATFORM_NAME);
    LOG_INFO(String("Version: ") + VERSION);

    // Initialize core
    if (!core::Initialize()) {
      LOG_ERROR("Failed to initialize CMS Core");
      return;
    }

    // Configure the system
    Config::Instance().Set("app.name", "CMS Client");
    Config::Instance().Set("app.type", "client");

    LOG_INFO(String("System Info: ") + core::GetSystemInfo());
    LOG_INFO(String("Config - App Name: ") +
             Config::Instance().GetOr("app.name", "N/A"));

    // Here usually we would start the networking/client logic loops
    // For now, we simulate a running state
  };

  auto stopApplication = []() {
    LOG_INFO("Stopping CMS Client...");
    core::Shutdown();
  };

#ifdef _WIN32
  if (runAsService) {
    // Run as Windows Service
    platform::WindowsService::Run(
        "ClassroomControlClient",
        [&]() {
          runApplication();
          // ServiceMain runs in a separate thread/context, but for our simple
          // app we might want to keep it alive. In a real app, runApplication
          // would start threads. Since runApplication currently just logs and
          // returns, we need a loop here or let the service helper handle
          // blocking? WindowsService::ServiceMain blocks until usage? Actually,
          // WindowsService::Run blocks? The current WindowsService
          // implementation: ServiceMain calls callback then sets STOPPED. So we
          // need a blocking loop here.

          // Simple blocking loop for now until we have a real event loop
          while (true) {
            Sleep(1000);
            // In a real implementation we would check a running flag
          }
        },
        [&]() {
          stopApplication();
          // Break the loop above (need thread synchro/atomic bool)
          exit(0); // Quick and dirty exit for MVP
        });
    return 0;
  }
#endif

  // Console Mode
  runApplication();

  std::cout << "\nCMS Client is running (Console Mode). Press Enter to exit..."
            << std::endl;
  std::cin.get();

  stopApplication();
  LOG_INFO("=== CMS Client Stopped ===");

  return static_cast<int>(StatusCode::Success);
}
