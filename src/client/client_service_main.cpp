#include "cms/ClientService.h"
#include "cms/Common.h"
#include "cms/Config.h"
#include "cms/Logger.h"
#include "cms/ServiceLauncher.h"
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

// Global service launcher
std::unique_ptr<cms::service::ServiceLauncher> g_serviceLauncher;
std::atomic<bool> g_serviceRunning{true};

// Service main callback
void ServiceMainCallback() {
  cms::Logger::getInstance().init("client_service_log.txt", true);
  cms::Logger::getInstance().SetLogLevel(cms::LogLevel::Debug);

  LOG_INFO("=== CMS Client Service Starting ===");

  try {
    // Initialize core
    if (!cms::core::Initialize()) {
      LOG_ERROR("Failed to initialize CMS Core");
      return;
    }

    LOG_INFO("CMS Core initialized");

    // Configure worker
    cms::service::WorkerConfig workerConfig;

    // Get executable directory
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = std::string(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));

    workerConfig.executable_path = exeDir + "\\cms_client_worker.exe";
    workerConfig.config_file_path = exeDir + "\\client_config.json";
    workerConfig.log_file_path = "C:\\Users\\Public\\cms_worker.log";
    workerConfig.restart_delay_ms = 2000;
    workerConfig.max_restart_attempts = 5;
    workerConfig.heartbeat_timeout_ms = 10000;

    LOG_INFO("Worker config:");
    LOG_INFO("  Executable: " + workerConfig.executable_path);
    LOG_INFO("  Config: " + workerConfig.config_file_path);

    // Create and start service launcher
    g_serviceLauncher =
        std::make_unique<cms::service::ServiceLauncher>(workerConfig);

    if (!g_serviceLauncher->Start()) {
      LOG_ERROR("Failed to start ServiceLauncher");
      return;
    }

    LOG_INFO("ServiceLauncher started successfully");

    // Main service loop
    while (g_serviceRunning && g_serviceLauncher->IsRunning()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    LOG_INFO("Service main loop exited");

  } catch (const std::exception &e) {
    LOG_ERROR("Exception in service main: " + std::string(e.what()));
  }

  LOG_INFO("=== CMS Client Service Stopped ===");
}

// Service stop callback
void ServiceStopCallback() {
  LOG_INFO("Service stop requested");
  g_serviceRunning = false;

  if (g_serviceLauncher) {
    g_serviceLauncher->Stop();
    g_serviceLauncher.reset();
  }

  cms::core::Shutdown();
}

int main(int argc, char *argv[]) {
  // Start as Windows service
  cms::platform::WindowsService::Run("WatcherClientService",
                                     ServiceMainCallback, ServiceStopCallback);

  return 0;
}

#else
#error "Client service is only supported on Windows"
#endif
