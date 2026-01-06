#include "cms/ServiceLauncher.h"
#include "cms/Logger.h"
#include <sstream>
#include <userenv.h>
#include <wtsapi32.h>


#ifdef _WIN32

namespace cms {
namespace service {

ServiceLauncher::ServiceLauncher(const WorkerConfig &config)
    : config_(config), workerProcessHandle_(INVALID_HANDLE_VALUE),
      workerThreadHandle_(INVALID_HANDLE_VALUE), workerPID_(0),
      consecutiveRestarts_(0) {}

ServiceLauncher::~ServiceLauncher() { Stop(); }

bool ServiceLauncher::Start() {
  if (running_) {
    LOG_WARNING("ServiceLauncher already running");
    return true;
  }

  LOG_INFO("Starting ServiceLauncher");
  state_ = LauncherState::Starting;
  running_ = true;

  // Create IPC server
  ipcServer_ =
      std::make_unique<ipc::NamedPipeServer>(ipc::PipeNames::CLIENT_SERVICE);

  // Set message handler
  ipcServer_->SetMessageHandler(
      [this](const ipc::IPCMessage &msg) { HandleWorkerMessage(msg); });

  // Start IPC server
  if (!ipcServer_->Start()) {
    LOG_ERROR("Failed to start IPC server");
    state_ = LauncherState::Error;
    return false;
  }

  LOG_INFO("IPC server started");

  // Spawn worker process
  if (!SpawnWorker()) {
    LOG_ERROR("Failed to spawn worker");
    state_ = LauncherState::Error;
    return false;
  }

  // Start monitor and heartbeat threads
  monitorThread_ =
      std::make_unique<std::thread>(&ServiceLauncher::MonitorLoop, this);
  heartbeatThread_ =
      std::make_unique<std::thread>(&ServiceLauncher::HeartbeatLoop, this);

  state_ = LauncherState::Running;
  LOG_INFO("ServiceLauncher started successfully");

  return true;
}

void ServiceLauncher::Stop() {
  if (!running_) {
    return;
  }

  LOG_INFO("Stopping ServiceLauncher");
  state_ = LauncherState::Stopping;
  running_ = false;

  // Send shutdown message to worker
  ipc::IPCMessage shutdownMsg =
      ipc::IPCMessage::Create(ipc::IPCMessageType::SERVICE_SHUTDOWN);
  SendMessageToWorker(shutdownMsg);

  // Wait a bit for graceful shutdown
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // Kill worker if still alive
  KillWorker();

  // Stop IPC server
  if (ipcServer_) {
    ipcServer_->Stop();
    ipcServer_.reset();
  }

  // Wait for threads to finish
  if (monitorThread_ && monitorThread_->joinable()) {
    monitorThread_->join();
  }

  if (heartbeatThread_ && heartbeatThread_->joinable()) {
    heartbeatThread_->join();
  }

  state_ = LauncherState::Stopped;
  LOG_INFO("ServiceLauncher stopped");
}

uint32_t ServiceLauncher::GetWorkerPID() const { return workerPID_; }

bool ServiceLauncher::RestartWorker() {
  LOG_INFO("Manual worker restart requested");
  return RestartWorkerInternal();
}

bool ServiceLauncher::SendMessageToWorker(const ipc::IPCMessage &message) {
  if (!ipcServer_) {
    LOG_ERROR("Cannot send message: IPC server not initialized");
    return false;
  }

  return ipcServer_->SendIPCMessage(message);
}

bool ServiceLauncher::SpawnWorker() {
  LOG_INFO("Spawning worker process: " + config_.executable_path);

  // Get the active console session ID
  DWORD sessionId = WTSGetActiveConsoleSessionId();
  if (sessionId == 0xFFFFFFFF) {
    LOG_ERROR("No active console session found");
    return false;
  }

  LOG_INFO("Active session ID: " + std::to_string(sessionId));

  HANDLE hToken = nullptr;
  if (!WTSQueryUserToken(sessionId, &hToken)) {
    DWORD error = GetLastError();
    LOG_ERROR("WTSQueryUserToken failed: " + std::to_string(error));
    return false;
  }

  HANDLE hDupToken = nullptr;
  if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, nullptr,
                        SecurityIdentification, TokenPrimary, &hDupToken)) {
    LOG_ERROR("DuplicateTokenEx failed: " + std::to_string(GetLastError()));
    CloseHandle(hToken);
    return false;
  }

  void *lpEnvironment = nullptr;
  if (!CreateEnvironmentBlock(&lpEnvironment, hDupToken, FALSE)) {
    LOG_ERROR("CreateEnvironmentBlock failed");
    CloseHandle(hDupToken);
    CloseHandle(hToken);
    return false;
  }

  // Build command line
  std::stringstream cmdLine;
  cmdLine << "\"" << config_.executable_path << "\"";
  cmdLine << " --config \"" << config_.config_file_path << "\"";

  std::string cmdLineStr = cmdLine.str();

  STARTUPINFOA si = {sizeof(si)};
  si.lpDesktop = (LPSTR) "winsta0\\default";
  PROCESS_INFORMATION pi = {0};

  // Create worker process as user
  BOOL success = CreateProcessAsUserA(
      hDupToken,
      NULL,                                   // Application name
      const_cast<char *>(cmdLineStr.c_str()), // Command line
      NULL,                                   // Process security attributes
      NULL,                                   // Thread security attributes
      FALSE,                                  // Inherit handles
      NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE |
          CREATE_UNICODE_ENVIRONMENT, // Creation flags
      lpEnvironment,                  // Environment
      NULL,                           // Current directory
      &si,                            // Startup info
      &pi                             // Process information
  );

  DestroyEnvironmentBlock(lpEnvironment);
  CloseHandle(hDupToken);
  CloseHandle(hToken);

  if (!success) {
    DWORD error = GetLastError();
    LOG_ERROR("CreateProcessAsUser failed: " + std::to_string(error));
    return false;
  }

  workerProcessHandle_ = pi.hProcess;
  workerThreadHandle_ = pi.hThread;
  workerPID_ = pi.dwProcessId;

  LOG_INFO("Worker process spawned as user with PID: " +
           std::to_string(workerPID_));

  // Reset heartbeat timer
  lastHeartbeat_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

  return true;
}

bool ServiceLauncher::KillWorker() {
  if (workerProcessHandle_ == INVALID_HANDLE_VALUE) {
    return true; // Already dead
  }

  LOG_INFO("Killing worker process");

  BOOL terminated = TerminateProcess(workerProcessHandle_, 1);

  if (terminated) {
    WaitForSingleObject(workerProcessHandle_, 5000);
  }

  CloseHandle(workerProcessHandle_);
  CloseHandle(workerThreadHandle_);

  workerProcessHandle_ = INVALID_HANDLE_VALUE;
  workerThreadHandle_ = INVALID_HANDLE_VALUE;
  workerPID_ = 0;

  return true;
}

bool ServiceLauncher::IsWorkerAlive() {
  if (workerProcessHandle_ == INVALID_HANDLE_VALUE) {
    return false;
  }

  DWORD exitCode = 0;
  if (GetExitCodeProcess(workerProcessHandle_, &exitCode)) {
    return exitCode == STILL_ACTIVE;
  }

  return false;
}

void ServiceLauncher::MonitorLoop() {
  LOG_INFO("Monitor loop started");

  while (running_) {
    std::this_thread::sleep_for(std::chrono::seconds(5));

    if (!IsWorkerAlive()) {
      LOG_WARNING("Worker process died unexpectedly");

      // Attempt restart
      if (running_) {
        RestartWorkerInternal();
      }
    }
  }

  LOG_INFO("Monitor loop stopped");
}

void ServiceLauncher::HeartbeatLoop() {
  LOG_INFO("Heartbeat loop started");

  while (running_) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(config_.heartbeat_timeout_ms));

    // Check if heartbeat is stale
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

    int64_t lastHb = lastHeartbeat_.load();
    int64_t timeSinceHeartbeat = now - lastHb;

    if (timeSinceHeartbeat > config_.heartbeat_timeout_ms * 2) {
      LOG_WARNING("Worker heartbeat timeout (last: " +
                  std::to_string(timeSinceHeartbeat) + "ms ago)");

      // Worker is unresponsive, restart it
      if (running_) {
        RestartWorkerInternal();
      }
    }
  }

  LOG_INFO("Heartbeat loop stopped");
}

void ServiceLauncher::HandleWorkerMessage(const ipc::IPCMessage &message) {
  switch (message.type) {
  case ipc::IPCMessageType::PROCESS_READY:
    LOG_INFO("Worker process ready");
    consecutiveRestarts_ = 0; // Reset restart counter

    // Send configuration to worker
    {
      nlohmann::json configPayload;
      configPayload["config_file"] = config_.config_file_path;

      ipc::IPCMessage configMsg = ipc::IPCMessage::Create(
          ipc::IPCMessageType::SERVICE_CONFIG, configPayload);
      SendMessageToWorker(configMsg);
    }
    break;

  case ipc::IPCMessageType::PROCESS_STATUS:
    // Update heartbeat
    lastHeartbeat_ = message.timestamp;
    break;

  case ipc::IPCMessageType::PROCESS_SHUTDOWN_ACK:
    LOG_INFO("Worker acknowledged shutdown");
    break;

  default:
    LOG_WARNING("Unhandled IPC message type: " +
                ipc::IPCMessageTypeToString(message.type));
    break;
  }
}

bool ServiceLauncher::RestartWorkerInternal() {
  LOG_INFO("Restarting worker process");

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

  if (consecutiveRestarts_ > config_.max_restart_attempts) {
    LOG_ERROR("Too many consecutive restarts (" +
              std::to_string(consecutiveRestarts_) + "), giving up");
    state_ = LauncherState::Error;
    return false;
  }

  // Kill existing worker
  KillWorker();

  // Wait before restarting (exponential backoff)
  uint32_t delayMs = config_.restart_delay_ms * consecutiveRestarts_;
  LOG_INFO("Waiting " + std::to_string(delayMs) + "ms before restart");
  std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

  // Spawn new worker
  return SpawnWorker();
}

} // namespace service
} // namespace cms

#endif // _WIN32
