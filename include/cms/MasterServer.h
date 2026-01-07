#ifndef CMS_MASTER_SERVER_H
#define CMS_MASTER_SERVER_H

#include "Common.h"
#include "cms/DomainPolicy.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace cms {
namespace master {

// ============================================================================
// DATA STRUCTURES
// ============================================================================

enum class ClientState {
  DISCONNECTED,
  CONNECTED,
  LOCKED,
  UNLOCKED,
  ERROR_STATE
};

struct ClientInfo {
  std::string id;         // Unique ID (e.g. UUID)
  std::string hostname;   // Machine name
  std::string ip_address; // IPv4
  std::string os_info;    // "Windows 10", "Ubuntu 22.04"
  ClientState state;      // Current connection/lock state
  time_t last_heartbeat;  // Timestamp of last keep-alive

  // Thumbnail data (simplified for now)
  std::vector<uint8_t> thumbnail_data;
  int thumbnail_width = 0;
  int thumbnail_height = 0;

  bool isConnected() const { return state != ClientState::DISCONNECTED; }
};

struct ServerConfig {
  uint16_t port = 5555;
  int max_clients = 30;
  std::string log_file_path;
  bool enable_logging = true;
};

// ============================================================================
// MASTER SERVER INTERFACE
// ============================================================================

class MasterServer {
public:
  virtual ~MasterServer() = default;

  // Lifecycle
  virtual bool start() = 0;
  virtual bool stop() = 0;
  virtual bool isRunning() const = 0;

  // Client Management
  virtual std::vector<ClientInfo> getConnectedClients() const = 0;
  virtual ClientInfo getClientInfo(const std::string &client_id) const = 0;

  // Commands
  virtual bool lockClient(const std::string &client_id) = 0;
  virtual bool unlockClient(const std::string &client_id) = 0;
  virtual bool lockAll() = 0;
  virtual bool unlockAll() = 0;

  virtual bool broadcastMessage(const std::string &message) = 0;

  // Request instant screenshot update
  virtual bool refreshClientThumbnail(const std::string &client_id) = 0;

  // Domain Policy
  virtual bool sendDomainPolicy(const std::string &client_id,
                                const DomainPolicy &policy) = 0;

  // Remote Input
  virtual bool sendRemoteInput(const std::string &client_id,
                               const nlohmann::json &input_data) = 0;

  // Power Control
  virtual bool sendPowerControl(const std::string &client_id,
                                const std::string &action) = 0;

  // Text Message
  virtual bool sendTextMessage(const std::string &client_id,
                               const std::string &title,
                               const std::string &message) = 0;

  // Remote Execute
  virtual bool sendRemoteExecute(const std::string &client_id,
                                 const std::string &command,
                                 const std::string &args) = 0;

  // Demo Mode
  virtual bool startDemoMode() = 0;
  virtual bool stopDemoMode() = 0;
  virtual bool isDemoModeActive() const = 0;
  virtual bool broadcastScreenFrame(const std::vector<uint8_t> &frameData,
                                    int width, int height) = 0;

  // File Transfer
  virtual bool sendFile(const std::string &client_id,
                        const std::string &filename,
                        const std::vector<uint8_t> &fileContent) = 0;

  // Observer Interface
  class IServerObserver {
  public:
    virtual ~IServerObserver() = default;
    virtual void onClientConnected(const ClientInfo &) {}
    virtual void onClientDisconnected(const std::string &) {}
    virtual void onClientStateChanged(const std::string &, ClientState) {}
    virtual void onClientThumbnailUpdated(const std::string &,
                                          const std::vector<uint8_t> &,
                                          int /*width*/, int /*height*/) {}
    virtual void onScreenshotReceived(const std::string &,
                                      const std::vector<uint8_t> &,
                                      int /*width*/, int /*height*/) {}
  };

  virtual void addObserver(IServerObserver *observer) = 0;
  virtual void removeObserver(IServerObserver *observer) = 0;
};

// Factory
std::unique_ptr<MasterServer> createMasterServer(const ServerConfig &config);

} // namespace master
} // namespace cms

#endif // CMS_MASTER_SERVER_H
