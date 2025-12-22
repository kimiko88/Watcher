#ifndef CMS_MASTER_SERVICE_H
#define CMS_MASTER_SERVICE_H

#include "Common.h"
#include "IPCChannel.h"
#include "IPCProtocol.h"
#include "MasterServer.h"
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

// Master Service State
enum class MasterServiceState { Stopped, Starting, Running, Stopping, Error };

// GUI Process Configuration
struct GUIConfig {
  std::string executable_path;   // Path to GUI executable (cms_master.exe)
  uint32_t restart_delay_ms;     // Delay before GUI restart (ms)
  uint32_t max_restart_attempts; // Max consecutive restart attempts
  bool auto_start_gui;           // Auto-start GUI on service start
};

// Master Service
// Manages MasterServer and optionally spawns/manages GUI process
class MasterService {
public:
  // Constructor
  explicit MasterService(const master::ServerConfig &serverConfig,
                         const GUIConfig &guiConfig);

  // Destructor
  ~MasterService();

  // Start the service (starts server and IPC)
  bool Start();

  // Stop the service
  void Stop();

  // Get current state
  MasterServiceState GetState() const { return state_; }

  // Check if running
  bool IsRunning() const { return state_ == MasterServiceState::Running; }

  // Start GUI process
  bool StartGUI();

  // Stop GUI process
  bool StopGUI();

  // Check if GUI is running
  bool IsGUIRunning() const;

  // Send server state update to GUI via IPC
  void NotifyGUIStateUpdate();

private:
  master::ServerConfig serverConfig_;
  GUIConfig guiConfig_;
  std::atomic<MasterServiceState> state_{MasterServiceState::Stopped};

  // Master server instance (runs in service)
  std::unique_ptr<master::MasterServer> masterServer_;

  // IPC for communication with GUI
  std::unique_ptr<ipc::NamedPipeServer> ipcServer_;

  // GUI process handles
  HANDLE guiProcessHandle_;
  HANDLE guiThreadHandle_;
  uint32_t guiPID_;

  // Threads
  std::unique_ptr<std::thread> serverThread_;
  std::unique_ptr<std::thread> guiMonitorThread_;
  std::atomic<bool> running_{false};

  // Restart tracking
  uint32_t consecutiveRestarts_;
  std::chrono::steady_clock::time_point lastRestartTime_;

  // Spawn GUI process
  bool SpawnGUI();

  // Kill GUI process
  bool KillGUI();

  // Check if GUI process is alive
  bool IsGUIAlive();

  // Monitor loop for GUI process
  void GUIMonitorLoop();

  // Handle IPC message from GUI
  void HandleGUIMessage(const ipc::IPCMessage &message);

  // Server thread (runs MasterServer)
  void ServerLoop();

  // Restart GUI with backoff
  bool RestartGUIInternal();

  // Serialize server state for IPC
  nlohmann::json SerializeServerState();
};

} // namespace service
} // namespace cms

#endif // CMS_MASTER_SERVICE_H
