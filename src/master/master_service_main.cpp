#include "cms/Common.h"
#include "cms/Config.h"
#include "cms/Logger.h"
#include "cms/MasterServer.h"
#include "cms/MasterService.h"
#include "cms/platform/WindowsService.h"
#include <atomic>
#include <iostream>
#include <memory>

namespace cms {
namespace core {
bool Initialize();
void Shutdown();
} // namespace core
} // namespace cms

#ifdef _WIN32

// Global master service
std::unique_ptr<cms::service::MasterService> g_masterService;
std::atomic<bool> g_serviceRunning{true};

// Service main callback
void ServiceMainCallback() {
  cms::Logger::getInstance().init("master_service_log.txt", true);
  cms::Logger::getInstance().SetLogLevel(cms::LogLevel::Debug);

  LOG_INFO("=== CMS Master Service Starting ===");

  try {
    // Initialize core
    if (!cms::core::Initialize()) {
      LOG_ERROR("Failed to initialize CMS Core");
      return;
    }

    LOG_INFO("CMS Core initialized");

    // Configure master server
    cms::master::ServerConfig serverConfig;
    serverConfig.port = 5555;
    serverConfig.max_clients = 100;

    // Configure GUI
    cms::service::GUIConfig guiConfig;

    // Get executable directory
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = std::string(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));

    guiConfig.executable_path = exeDir + "\\cms_master.exe";
    guiConfig.restart_delay_ms = 2000;
    guiConfig.max_restart_attempts = 5;
    guiConfig.auto_start_gui = false; // Don't auto-start GUI by default

    LOG_INFO("Master service config:");
    LOG_INFO("  GUI executable: " + guiConfig.executable_path);
    LOG_INFO("  Server port: " + std::to_string(serverConfig.port));

    // Create and start master service
    g_masterService =
        std::make_unique<cms::service::MasterService>(serverConfig, guiConfig);

    if (!g_masterService->Start()) {
      LOG_ERROR("Failed to start MasterService");
      return;
    }

    LOG_INFO("MasterService started successfully");

    // Main service loop
    while (g_serviceRunning && g_masterService->IsRunning()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    LOG_INFO("Service main loop exited");

  } catch (const std::exception &e) {
    LOG_ERROR("Exception in service main: " + std::string(e.what()));
  }

  LOG_INFO("=== CMS Master Service Stopped ===");
}

// Service stop callback
void ServiceStopCallback() {
  LOG_INFO("Service stop requested");
  g_serviceRunning = false;

  if (g_masterService) {
    g_masterService->Stop();
    g_masterService.reset();
  }

  cms::core::Shutdown();
}

int main(int argc, char *argv[]) {
  // Start as Windows service
  cms::platform::WindowsService::Run("WatcherMasterService",
                                     ServiceMainCallback, ServiceStopCallback);

  return 0;
}

#else
#error "Master service is only supported on Windows"
#endif
