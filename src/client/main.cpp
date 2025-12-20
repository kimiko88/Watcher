#include "cms/ClientService.h"
#include "cms/Common.h"
#include "cms/Config.h"
#include "cms/Logger.h"
#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>


namespace cms {
namespace core {
bool Initialize();
void Shutdown();
String GetSystemInfo();
} // namespace core
} // namespace cms

#ifdef _WIN32
#include "cms/platform/WindowsService.h"
#include <windows.h>
#endif

// === Enhanced Debug Logging ===
void LogMain(const std::string &msg) {
  std::ofstream f("C:\\Users\\Public\\cms_debug.txt", std::ios::app);
  if (f.is_open()) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S",
                  std::localtime(&time_t));
    f << "[" << buf << "] [MAIN] " << msg << std::endl;
  }
}

// === Global State ===
std::unique_ptr<cms::client::ClientService> g_clientService;
std::atomic<bool> g_running{true};

// === Signal Handler ===
void signalHandler(int signal) {
  LogMain("Signal received: " + std::to_string(signal));
  g_running = false;
}

int main(int argc, char *argv[]) {
  using namespace cms;

  LogMain("========================================");
  LogMain("CMS Client Starting");
  LogMain("Arguments: " + std::to_string(argc));
  for (int i = 0; i < argc; i++) {
    LogMain("  [" + std::to_string(i) + "]: " + std::string(argv[i]));
  }

  // === Parse Arguments ===
  bool runAsService = false;
  std::string configPath = "client_config.json";

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--service") {
      runAsService = true;
    } else if (arg == "--config" && i + 1 < argc) {
      configPath = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: cms_client [options]\n"
                << "Options:\n"
                << "  --service          Run as Windows service\n"
                << "  --config <path>    Config file path (default: "
                   "client_config.json)\n"
                << "  --help, -h         Show this help\n";
      return 0;
    }
  }

  LogMain("Mode: " + std::string(runAsService ? "Service" : "Console"));
  LogMain("Config: " + configPath);

  // === Setup Signal Handlers ===
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  // === Configure Logging ===
  Logger::getInstance().SetLogLevel(LogLevel::Debug);
  LogMain("Logger initialized with Debug level");

  // === Main Application Logic ===
  auto runApplication = [&configPath]() {
    try {
      LOG_INFO("=== CMS Client Starting ===");
      LOG_INFO("Platform: " + std::string(PLATFORM_NAME));
      LOG_INFO("Version: " + std::string(VERSION));
      LOG_INFO("Config file: " + configPath);
      LogMain("Application logic starting");

      // Initialize core
      LOG_INFO("Initializing CMS Core...");
      if (!core::Initialize()) {
        LOG_ERROR("Failed to initialize CMS Core");
        LogMain("ERROR: Core initialization failed");
        return;
      }
      LOG_INFO("CMS Core initialized");
      LogMain("Core initialized successfully");

      // Configure system
      Config::Instance().Set("app.name", "CMS Client");
      Config::Instance().Set("app.type", "client");
      LOG_INFO("System Info: " + core::GetSystemInfo());

      // === CREATE AND START CLIENT SERVICE ===
      LOG_INFO("Creating ClientService with config: " + configPath);
      LogMain("Creating ClientService...");

      // Check if config file exists
      std::ifstream configCheck(configPath);
      if (!configCheck.good()) {
        LOG_ERROR("Config file not found: " + configPath);
        LogMain("ERROR: Config file not found: " + configPath);
        return;
      }
      configCheck.close();

      g_clientService =
          std::make_unique<cms::client::ClientService>(configPath);
      LOG_INFO("ClientService created");
      LogMain("ClientService instance created");

      // Start the service
      LOG_INFO("Starting ClientService...");
      LogMain("Calling ClientService::start()...");

      if (!g_clientService->start()) {
        LOG_ERROR("Failed to start ClientService");
        LogMain("ERROR: ClientService::start() returned false");
        return;
      }

      LOG_INFO("ClientService started successfully!");
      LogMain("ClientService started - entering main loop");

      // === Main Loop ===
      int statusLogCounter = 0;
      while (g_running && g_clientService->isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Log status every 30 seconds
        if (++statusLogCounter >= 30) {
          statusLogCounter = 0;
          auto status = g_clientService->getStatus();

          std::string statusMsg =
              "Status: " +
              std::string(status.is_connected ? "CONNECTED" : "DISCONNECTED") +
              " | Uptime: " + std::to_string(status.uptime) + "s" +
              " | Pending: " +
              std::to_string(g_clientService->getPendingCommandCount()) +
              " | MachineID: " + status.machine_id;

          LOG_INFO(statusMsg);
          LogMain(statusMsg);
        }
      }

      LOG_INFO("Main loop exited (running=" + std::to_string(g_running.load()) +
               ")");
      LogMain("Main loop exited");

    } catch (const std::exception &e) {
      std::string errMsg =
          "EXCEPTION in runApplication: " + std::string(e.what());
      LOG_ERROR(errMsg);
      LogMain(errMsg);
    } catch (...) {
      LOG_ERROR("UNKNOWN EXCEPTION in runApplication");
      LogMain("Unknown exception caught");
    }
  };

  auto stopApplication = []() {
    LOG_INFO("Stopping CMS Client...");
    LogMain("Stop requested");

    g_running = false;

    if (g_clientService) {
      LOG_INFO("Stopping ClientService...");
      g_clientService->stop();
      g_clientService.reset();
      LOG_INFO("ClientService stopped");
      LogMain("ClientService destroyed");
    }

    LOG_INFO("Shutting down core...");
    core::Shutdown();
    LOG_INFO("=== CMS Client Stopped ===");
    LogMain("Application stopped");
  };

#ifdef _WIN32
  if (runAsService) {
    LogMain("Initializing Windows Service");
    platform::WindowsService::Run(
        "ClassroomControlClient",
        [&]() {
          LogMain("Service main callback invoked");
          runApplication();
        },
        [&]() {
          LogMain("Service stop callback invoked");
          stopApplication();
        });
    return 0;
  }
#endif

  // === Console Mode ===
  LogMain("Starting in console mode");

  std::thread appThread(runApplication);

  std::cout << "\n========================================\n";
  std::cout << "   CMS Client Running (Console Mode)   \n";
  std::cout << "========================================\n";
  std::cout << "Config: " << configPath << "\n";
  std::cout << "Logs: C:\\Users\\Public\\cms_debug.txt\n";
  std::cout << "Status updates every 30 seconds\n";
  std::cout << "========================================\n";
  std::cout << "Press Enter to exit...\n";

  std::cin.get();

  std::cout << "\nShutting down...\n";
  LogMain("User requested shutdown via console");
  stopApplication();

  if (appThread.joinable()) {
    appThread.join();
  }

  LogMain("========================================");
  LogMain("CMS Client Terminated");
  LogMain("========================================");

  return static_cast<int>(StatusCode::Success);
}