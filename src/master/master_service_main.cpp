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

  LOG_WARNING("Master service currently runs in stub mode.");
  LOG_WARNING("Use cms_master.exe GUI for full master functionality.");

  // NOTE: MasterServer requires Qt dependencies (QTcpServer, QHostAddress)
  // which cannot be linked in service without Qt.
  // For full functionality, use cms_master.exe GUI application.

  LOG_INFO("Master service stub running (no server functionality)");
  LOG_INFO("To use master features, run cms_master.exe instead");

  // Keep service alive
  while (g_serviceRunning) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
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
