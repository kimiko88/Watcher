#include "cms/ClientService.h"
#include "cms/Common.h"
#include "cms/Config.h"
#include "cms/IPCChannel.h"
#include "cms/IPCProtocol.h"
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
#include <windows.h>
#endif

// === Enhanced Debug Logging ===
void LogMain(const std::string &msg) {
  std::ofstream f("C:\\Users\\Public\\cms_worker_debug.txt", std::ios::app);
  if (f.is_open()) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S",
                  std::localtime(&time_t));
    f << "[" << buf << "] [WORKER] " << msg << std::endl;
  }
}

// === Global State ===
std::unique_ptr<cms::client::ClientService> g_clientService;
std::unique_ptr<cms::ipc::NamedPipeClient> g_ipcClient;
std::atomic<bool> g_running{true};

// === Signal Handler ===
void signalHandler(int signal) {
  LogMain("Signal received: " + std::to_string(signal));
  g_running = false;
}

int main(int argc, char *argv[]) {
  using namespace cms;

  LogMain("========================================");
  LogMain("CMS Client Worker Starting");
  LogMain("Arguments: " + std::to_string(argc));
  for (int i = 0; i < argc; i++) {
    LogMain("  [" + std::to_string(i) + "]: " + std::string(argv[i]));
  }

  // === Parse Arguments ===
  std::string configPath = "client_config.json";

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      configPath = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: cms_client_worker [options]\n"
                << "Options:\n"
                << "  --config <path>    Config file path (default: "
                   "client_config.json)\n"
                << "  --help, -h         Show this help\n";
      return 0;
    }
  }

  LogMain("Config: " + configPath);

  // === Setup Signal Handlers ===
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  // === Configure Logging ===
  Logger::getInstance().SetLogLevel(LogLevel::Debug);
  LogMain("Logger initialized with Debug level");

  // === Connect to Service via IPC ===
  LOG_INFO("=== CMS Client Worker Starting ===");
  LOG_INFO("Connecting to service via IPC...");
  LogMain("Connecting to service IPC");

  g_ipcClient =
      std::make_unique<ipc::NamedPipeClient>(ipc::PipeNames::CLIENT_SERVICE);

  if (!g_ipcClient->Connect()) {
    LOG_ERROR("Failed to connect to service");
    LogMain("ERROR: Failed to connect to service");
    return static_cast<int>(StatusCode::Error);
  }

  LOG_INFO("Connected to service");
  LogMain("IPC connection established");

  // Set up IPC message handler
  g_ipcClient->SetMessageHandler([&](const ipc::IPCMessage &msg) {
    LOG_INFO("Received IPC message: " + ipc::IPCMessageTypeToString(msg.type));

    switch (msg.type) {
    case ipc::IPCMessageType::SERVICE_CONFIG:
      LOG_INFO("Received configuration from service");
      if (msg.payload.contains("config_file")) {
        configPath = msg.payload["config_file"].get<std::string>();
        LOG_INFO("Config path updated: " + configPath);
      }
      break;

    case ipc::IPCMessageType::SERVICE_SHUTDOWN:
      LOG_INFO("Shutdown requested by service");
      g_running = false;

      // Send acknowledgment
      {
        ipc::IPCMessage ack =
            ipc::IPCMessage::Create(ipc::IPCMessageType::PROCESS_SHUTDOWN_ACK);
        g_ipcClient->SendIPCMessage(ack);
      }
      break;

    case ipc::IPCMessageType::SERVICE_RESTART:
      LOG_INFO("Restart requested by service");
      g_running = false;
      break;

    default:
      LOG_WARNING("Unhandled IPC message: " +
                  ipc::IPCMessageTypeToString(msg.type));
      break;
    }
  });

  // Start reading IPC messages
  g_ipcClient->StartReading();

  // Send PROCESS_READY message
  {
    ipc::IPCMessage readyMsg =
        ipc::IPCMessage::Create(ipc::IPCMessageType::PROCESS_READY);
    if (!g_ipcClient->SendIPCMessage(readyMsg)) {
      LOG_ERROR("Failed to send PROCESS_READY message");
    } else {
      LOG_INFO("Sent PROCESS_READY message");
    }
  }

  // === Main Application Logic ===
  auto runApplication = [&configPath]() {
    try {
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
      Config::Instance().Set("app.name", "CMS Client Worker");
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

      // Set IPC client for delegation
      g_clientService->setIPCClient(g_ipcClient.get());

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

      // === Main Loop with IPC Heartbeat ===
      int statusLogCounter = 0;
      int heartbeatCounter = 0;

      while (g_running && g_clientService->isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Send heartbeat to service every 5 seconds
        if (++heartbeatCounter >= 5) {
          heartbeatCounter = 0;

          nlohmann::json statusPayload;
          auto status = g_clientService->getStatus();
          statusPayload["connected"] = status.is_connected;
          statusPayload["uptime"] = status.uptime;
          statusPayload["machine_id"] = status.machine_id;

          ipc::IPCMessage heartbeat = ipc::IPCMessage::Create(
              ipc::IPCMessageType::PROCESS_STATUS, statusPayload);

          if (!g_ipcClient->SendIPCMessage(heartbeat)) {
            LOG_WARNING("Failed to send heartbeat to service");
          }
        }

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
    LOG_INFO("Stopping CMS Client Worker...");
    LogMain("Stop requested");

    g_running = false;

    if (g_clientService) {
      LOG_INFO("Stopping ClientService...");
      g_clientService->stop();
      g_clientService.reset();
      LOG_INFO("ClientService stopped");
      LogMain("ClientService destroyed");
    }

    if (g_ipcClient) {
      LOG_INFO("Disconnecting from service...");
      g_ipcClient->Disconnect();
      g_ipcClient.reset();
      LOG_INFO("Disconnected from service");
    }

    LOG_INFO("Shutting down core...");
    core::Shutdown();
    LOG_INFO("=== CMS Client Worker Stopped ===");
    LogMain("Application stopped");
  };

  // === Run in Console Mode (Worker Mode) ===
  LogMain("Starting in worker mode");

  std::thread appThread(runApplication);

  std::cout << "\n========================================\n";
  std::cout << "   CMS Client Worker Running           \n";
  std::cout << "========================================\n";
  std::cout << "Config: " << configPath << "\n";
  std::cout << "Logs: C:\\Users\\Public\\cms_worker_debug.txt\n";
  std::cout << "IPC: Connected to service\n";
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