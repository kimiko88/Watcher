#include "cms/MasterService.h"
#include "cms/Logger.h"
#include <sstream>

#ifdef _WIN32

namespace cms {
namespace service {

MasterService::MasterService(const master::ServerConfig &serverConfig,
                             const GUIConfig &guiConfig)
    : serverConfig_(serverConfig), guiConfig_(guiConfig),
      guiProcessHandle_(INVALID_HANDLE_VALUE),
      guiThreadHandle_(INVALID_HANDLE_VALUE), guiPID_(0),
      consecutiveRestarts_(0) {}

MasterService::~MasterService() { Stop(); }

bool MasterService::Start() {
  if (running_) {
    LOG_WARNING("MasterService already running");
    return true;
  }

  LOG_INFO("Starting MasterService");
  state_ = MasterServiceState::Starting;
  running_ = true;

  // Create master server instance
  LOG_INFO("Creating MasterServer...");
  masterServer_ = master::createMasterServer(serverConfig_);

  if (!masterServer_) {
    LOG_ERROR("Failed to create MasterServer");
    state_ = MasterServiceState::Error;
    return false;
  }

  // Start master server in separate thread
  serverThread_ =
      std::make_unique<std::thread>(&MasterService::ServerLoop, this);

  // Create IPC server for GUI communication
  ipcServer_ =
      std::make_unique<ipc::NamedPipeServer>(ipc::PipeNames::MASTER_SERVICE);

  // Set message handler
  ipcServer_->SetMessageHandler(
      [this](const ipc::IPCMessage &msg) { HandleGUIMessage(msg); });

  // Start IPC server
  if (!ipcServer_->Start()) {
    LOG_ERROR("Failed to start IPC server");
    state_ = MasterServiceState::Error;
    return false;
  }

  LOG_INFO("IPC server started");

  // Auto-start GUI if configured
  if (guiConfig_.auto_start_gui) {
    if (!StartGUI()) {
      LOG_WARNING("Failed to auto-start GUI (service will continue)");
    }
  }

  state_ = MasterServiceState::Running;
  LOG_INFO("MasterService started successfully");

  return true;
}

void MasterService::Stop() {
  if (!running_) {
    return;
  }

  LOG_INFO("Stopping MasterService");
  state_ = MasterServiceState::Stopping;
  running_ = false;

  // Stop GUI if running
  StopGUI();

  // Stop IPC server
  if (ipcServer_) {
    ipcServer_->Stop();
    ipcServer_.reset();
  }

  // Stop master server
  if (masterServer_) {
    LOG_INFO("Stopping MasterServer...");
    masterServer_->stop();
    masterServer_.reset();
  }

  // Wait for server thread
  if (serverThread_ && serverThread_->joinable()) {
    serverThread_->join();
  }

  // Wait for GUI monitor thread
  if (guiMonitorThread_ && guiMonitorThread_->joinable()) {
    guiMonitorThread_->join();
  }

  state_ = MasterServiceState::Stopped;
  LOG_INFO("MasterService stopped");
}

bool MasterService::StartGUI() {
  if (IsGUIRunning()) {
    LOG_WARNING("GUI already running");
    return true;
  }

  LOG_INFO("Starting GUI process");

  if (!SpawnGUI()) {
    LOG_ERROR("Failed to spawn GUI");
    return false;
  }

  // Start GUI monitor thread
  guiMonitorThread_ =
      std::make_unique<std::thread>(&MasterService::GUIMonitorLoop, this);

  return true;
}

bool MasterService::StopGUI() {
  if (!IsGUIRunning()) {
    return true;
  }

  LOG_INFO("Stopping GUI process");

  // Send shutdown message via IPC
  ipc::IPCMessage shutdownMsg =
      ipc::IPCMessage::Create(ipc::IPCMessageType::SERVICE_SHUTDOWN);

  if (ipcServer_) {
    ipcServer_->SendMessage(shutdownMsg);
  }

  // Wait a bit for graceful shutdown
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // Kill GUI if still alive
  return KillGUI();
}

bool MasterService::IsGUIRunning() const {
  return guiProcessHandle_ != INVALID_HANDLE_VALUE && IsGUIAlive();
}

bool MasterService::SpawnGUI() {
  LOG_INFO("Spawning GUI process: " + guiConfig_.executable_path);

  // Build command line with IPC flag
  std::stringstream cmdLine;
  cmdLine << "\"" << guiConfig_.executable_path << "\"";
  cmdLine << " --ipc"; // Flag to indicate IPC mode

  std::string cmdLineStr = cmdLine.str();

  STARTUPINFOA si = {sizeof(si)};
  PROCESS_INFORMATION pi = {0};

  // Create GUI process
  BOOL success =
      CreateProcessA(NULL,                                   // Application name
                     const_cast<char *>(cmdLineStr.c_str()), // Command line
                     NULL,               // Process security attributes
                     NULL,               // Thread security attributes
                     FALSE,              // Inherit handles
                     CREATE_NEW_CONSOLE, // Creation flags
                     NULL,               // Environment
                     NULL,               // Current directory
                     &si,                // Startup info
                     &pi                 // Process information
      );

  if (!success) {
    DWORD error = GetLastError();
    LOG_ERROR("CreateProcess failed: " + std::to_string(error));
    return false;
  }

  guiProcessHandle_ = pi.hProcess;
  guiThreadHandle_ = pi.hThread;
  guiPID_ = pi.dwProcessId;

  LOG_INFO("GUI process spawned with PID: " + std::to_string(guiPID_));

  return true;
}

bool MasterService::KillGUI() {
  if (guiProcessHandle_ == INVALID_HANDLE_VALUE) {
    return true; // Already dead
  }

  LOG_INFO("Killing GUI process");

  BOOL terminated = TerminateProcess(guiProcessHandle_, 0);

  if (terminated) {
    WaitForSingleObject(guiProcessHandle_, 5000);
  }

  CloseHandle(guiProcessHandle_);
  CloseHandle(guiThreadHandle_);

  guiProcessHandle_ = INVALID_HANDLE_VALUE;
  guiThreadHandle_ = INVALID_HANDLE_VALUE;
  guiPID_ = 0;

  return true;
}

bool MasterService::IsGUIAlive() {
  if (guiProcessHandle_ == INVALID_HANDLE_VALUE) {
    return false;
  }

  DWORD exitCode = 0;
  if (GetExitCodeProcess(guiProcessHandle_, &exitCode)) {
    return exitCode == STILL_ACTIVE;
  }

  return false;
}

void MasterService::GUIMonitorLoop() {
  LOG_INFO("GUI monitor loop started");

  while (running_ && guiConfig_.auto_start_gui) {
    std::this_thread::sleep_for(std::chrono::seconds(5));

    if (!IsGUIAlive()) {
      LOG_WARNING("GUI process died unexpectedly");

      // Attempt restart if auto-start is enabled
      if (running_ && guiConfig_.auto_start_gui) {
        RestartGUIInternal();
      }
    }
  }

  LOG_INFO("GUI monitor loop stopped");
}

void MasterService::HandleGUIMessage(const ipc::IPCMessage &message) {
  switch (message.type) {
  case ipc::IPCMessageType::GUI_READY:
    LOG_INFO("GUI connected");
    consecutiveRestarts_ = 0; // Reset restart counter

    // Send initial server state
    NotifyGUIStateUpdate();
    break;

  case ipc::IPCMessageType::EXECUTE_COMMAND:
    LOG_INFO("GUI sent command");

    // Forward command to MasterServer
    // Commands could be: screenshot, lock, unlock, etc.
    if (message.payload.contains("command")) {
      std::string cmd = message.payload["command"];
      LOG_INFO("Executing command from GUI: " + cmd);

      // TODO: Execute command on MasterServer
      // For now, just log it
    }
    break;

  default:
    LOG_WARNING("Unhandled GUI message: " +
                ipc::IPCMessageTypeToString(message.type));
    break;
  }
}

void MasterService::ServerLoop() {
  LOG_INFO("Server loop started");

  try {
    // Start the master server
    if (masterServer_->start()) {
      LOG_INFO("MasterServer started successfully");

      // Server runs in its own event loop
      // Keep thread alive while running
      while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    } else {
      LOG_ERROR("Failed to start MasterServer");
      state_ = MasterServiceState::Error;
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Exception in server loop: " + std::string(e.what()));
    state_ = MasterServiceState::Error;
  }

  LOG_INFO("Server loop stopped");
}

bool MasterService::RestartGUIInternal() {
  LOG_INFO("Restarting GUI process");

  // Check restart rate limiting
  auto now = std::chrono::steady_clock::now();
  auto timeSinceLastRestart =
      std::chrono::duration_cast<std::chrono::seconds>(now - lastRestartTime_)
          .count();

  if (timeSinceLastRestart < 60) {
    consecutiveRestarts_++;
  } else {
    consecutiveRestarts_ = 1;
  }

  lastRestartTime_ = now;

  if (consecutiveRestarts_ > guiConfig_.max_restart_attempts) {
    LOG_ERROR("Too many consecutive GUI restarts (" +
              std::to_string(consecutiveRestarts_) + "), giving up");
    return false;
  }

  // Kill existing GUI
  KillGUI();

  // Wait before restarting (exponential backoff)
  uint32_t delayMs = guiConfig_.restart_delay_ms * consecutiveRestarts_;
  LOG_INFO("Waiting " + std::to_string(delayMs) + "ms before GUI restart");
  std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

  // Spawn new GUI
  return SpawnGUI();
}

void MasterService::NotifyGUIStateUpdate() {
  if (!ipcServer_) {
    return;
  }

  nlohmann::json statePayload = SerializeServerState();

  ipc::IPCMessage stateMsg =
      ipc::IPCMessage::Create(ipc::IPCMessageType::SERVER_STATE, statePayload);

  ipcServer_->SendMessage(stateMsg);
}

nlohmann::json MasterService::SerializeServerState() {
  nlohmann::json state;

  if (!masterServer_) {
    state["running"] = false;
    return state;
  }

  state["running"] = true;
  state["client_count"] = 0; // TODO: Get from MasterServer

  // TODO: Add more server state information
  // - Connected clients list
  // - Server configuration
  // - Statistics

  return state;
}

} // namespace service
} // namespace cms

#endif // _WIN32
