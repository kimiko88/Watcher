#ifndef CMS_MASTER_SERVER_H
#define CMS_MASTER_SERVER_H

#include "Common.h"
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

  // Observer Interface
  class IServerObserver {
  public:
    virtual ~IServerObserver() = default;
    virtual void onClientConnected(const ClientInfo &client) {}
    virtual void onClientDisconnected(const std::string &client_id) {}
    virtual void onClientStateChanged(const std::string &client_id,
                                      ClientState new_state) {}
    virtual void onClientThumbnailUpdated(const std::string &client_id,
                                          const std::vector<uint8_t> &data) {}
  };

  virtual void addObserver(IServerObserver *observer) = 0;
  virtual void removeObserver(IServerObserver *observer) = 0;
};

// Factory
std::unique_ptr<MasterServer> createMasterServer(const ServerConfig &config);

} // namespace master
} // namespace cms

#endif // CMS_MASTER_SERVER_H
