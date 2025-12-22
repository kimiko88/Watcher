#ifndef CMS_SERVICE_LAUNCHER_H
#define CMS_SERVICE_LAUNCHER_H

#include "Common.h"
#include "IPCChannel.h"
#include "IPCProtocol.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>


#ifdef _WIN32
#include <windows.h>
#endif

namespace cms {
namespace service {

// Service Launcher State
enum class LauncherState { Stopped, Starting, Running, Stopping, Error };

// Worker Process Configuration
struct WorkerConfig {
  std::string executable_path;   // Path to worker executable
  std::string config_file_path;  // Path to config file
  std::string log_file_path;     // Path to log file
  uint32_t restart_delay_ms;     // Delay before restart (ms)
  uint32_t max_restart_attempts; // Max consecutive restart attempts
  uint32_t heartbeat_timeout_ms; // Heartbeat timeout (ms)
};

// Client Service Launcher
// Manages spawning and monitoring of worker process
class ServiceLauncher {
public:
  // Constructor
  explicit ServiceLauncher(const WorkerConfig &config);

  // Destructor
  ~ServiceLauncher();

  // Start the launcher (creates IPC server and spawns worker)
  bool Start();

  // Stop the launcher (kills worker and stops IPC)
  void Stop();

  // Get current state
  LauncherState GetState() const { return state_; }

  // Check if running
  bool IsRunning() const { return state_ == LauncherState::Running; }

  // Get worker process ID
  uint32_t GetWorkerPID() const;

  // Request worker restart
  bool RestartWorker();

  // Send message to worker
  bool SendMessageToWorker(const ipc::IPCMessage &message);

private:
  WorkerConfig config_;
  std::atomic<LauncherState> state_{LauncherState::Stopped};

  // IPC for communication with worker
  std::unique_ptr<ipc::NamedPipeServer> ipcServer_;

  // Worker process handles
  HANDLE workerProcessHandle_;
  HANDLE workerThreadHandle_;
  uint32_t workerPID_;

  // Threads
  std::unique_ptr<std::thread> monitorThread_;
  std::unique_ptr<std::thread> heartbeatThread_;
  std::atomic<bool> running_{false};

  // Restart tracking
  uint32_t consecutiveRestarts_;
  std::chrono::steady_clock::time_point lastRestartTime_;

  // Last heartbeat from worker
  std::atomic<int64_t> lastHeartbeat_{0};

  // Spawn worker process
  bool SpawnWorker();

  // Kill worker process
  bool KillWorker();

  // Check if worker process is alive
  bool IsWorkerAlive();

  // Monitor loop (watches worker process)
  void MonitorLoop();

  // Heartbeat loop (checks worker heartbeat)
  void HeartbeatLoop();

  // Handle IPC message from worker
  void HandleWorkerMessage(const ipc::IPCMessage &message);

  // Restart worker with backoff
  bool RestartWorkerInternal();
};

} // namespace service
} // namespace cms

#endif // CMS_SERVICE_LAUNCHER_H
