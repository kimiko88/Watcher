#ifndef CMS_CLIENT_SERVICE_H
#define CMS_CLIENT_SERVICE_H

#include "Common.h"
#include "Protocol.h"
#include "cms/Platform.h"
#include "cms/Socket.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>


namespace cms {
namespace client {

// Client status information
struct ClientStatus {
  bool is_connected;           // Is connected to master
  int64_t last_heartbeat;      // Last heartbeat timestamp
  std::string service_version; // Client service version
  std::string machine_id;      // Unique machine identifier
  uint64_t uptime;             // Service uptime in seconds
};

// Client Service class
// Manages connection to master server and command processing
class ClientService {
public:
  // Constructor
  // Loads configuration from JSON file
  // Throws std::runtime_error if config is invalid
  explicit ClientService(const std::string &config_path);

  // Destructor
  // Stops service if running
  ~ClientService();

  // Start the client service
  // Connects to master and begins processing loop
  // Returns true if started successfully
  bool start();

  // Stop the client service
  // Disconnects from master and stops processing loop
  // Returns true if stopped successfully
  bool stop();

  // Check if service is running
  bool isRunning() const;

  // Get current service status
  ClientStatus getStatus() const;

  // Get heartbeat interval in seconds
  int getHeartbeatInterval() const;

  // Get maximum reconnection attempts
  int getMaxReconnectAttempts() const;

  // Get pending command count
  size_t getPendingCommandCount() const;

private:
  // Configuration structure
  struct Config {
    std::string master_address;
    int master_port;
    std::string machine_id;
    bool encryption_enabled;
    std::string log_level;
  };

  // Load configuration from file
  void loadConfig(const std::string &config_path);

  // Main processing loop (runs in separate thread)
  void processingLoop();

  // Connect to master server
  bool connectToMaster();

  // Disconnect from master
  void disconnect();

  // Send HELLO handshake message
  bool sendHello();

  // Send heartbeat message
  void sendHeartbeat();

  // Process incoming commands
  void processCommands();

  // Handle reconnection
  void handleReconnection();

  // Member variables
  Config config_;
  std::atomic<bool> running_{false};
  std::atomic<bool> connected_{false};
  std::atomic<int64_t> last_heartbeat_{0};
  std::atomic<uint64_t> start_time_{0};

  // Platform abstraction for system operations
  // Platform abstraction for system operations
  cms::platform::Platform *platform_ = nullptr;

  // Send screenshot to master
  void sendScreenshot();

  std::unique_ptr<std::thread> processing_thread_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;

  std::queue<protocol::Message> command_queue_;
  mutable std::mutex queue_mutex_;

  // Network socket
  std::unique_ptr<cms::Socket> socket_ = nullptr;

  // Constants
  static constexpr int DEFAULT_HEARTBEAT_INTERVAL = 30; // seconds
  static constexpr int MAX_RECONNECT_ATTEMPTS = 10;
  static constexpr int RECONNECT_DELAY = 5; // seconds
};

} // namespace client
} // namespace cms

#endif // CMS_CLIENT_SERVICE_H
