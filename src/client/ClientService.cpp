#include "cms/ClientService.h"
#include "cms/Logger.h"
#include "cms/NetworkFilterManager.h"
#include "cms/Platform.h"
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>

namespace cms {
namespace client {

// ============================================================================
// CONSTRUCTION / DESTRUCTION
// ============================================================================

// Helper for Base64 encoding
namespace {
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

std::string base64_encode(unsigned char const *bytes_to_encode, size_t in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] =
          ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] =
          ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4); i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] =
        ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] =
        ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while ((i++ < 3))
      ret += '=';
  }

  return ret;
}
} // namespace

ClientService::ClientService(const std::string &config_path) {
  loadConfig(config_path);
  // Initialize platform interface
  platform_ = platform::getPlatformInstance().release();

  // Initialize Socket subsystem
  cms::Socket::Initialize();

  // Initialize Network Filter Manager
  // We use a default config file for persistence
  networkFilter_ = std::make_unique<cms::network::NetworkFilterManager>(
      platform_, "domain_rules.json");
  // Load any existing rules
  networkFilter_->loadRules();
  // Apply them
  networkFilter_->applyRules();

  // Initialize Application Manager
  appManager_ = std::make_unique<cms::ApplicationManager>();
  // Load rules if persistence implemented (TODO: Add persistence file path)
  // appManager_->importRules("app_rules.csv");

  LOG_INFO("ClientService initialized with machine ID: " + config_.machine_id);
}

// ... (Destructor stays same)

// ... (Other methods stay same)

void ClientService::processCommands() {
  std::lock_guard<std::mutex> lock(queue_mutex_);

  // Process all queued commands
  while (!command_queue_.empty()) {
    auto cmd = command_queue_.front();
    command_queue_.pop();

    LOG_INFO("Processing command: " + protocol::CommandTypeToString(cmd.type));

    switch (cmd.type) {
    case protocol::CommandType::SCREENSHOT_REQUEST:
      sendScreenshot();
      break;
    case protocol::CommandType::SCREEN_LOCK:
      if (platform_)
        platform_->lockKeyboard(); // Simplified for now
      // In full implementation, invoke InputLockManager logic
      break;
    case protocol::CommandType::SCREEN_UNLOCK:
      if (platform_)
        platform_->unlockKeyboard();
      break;
    case protocol::CommandType::POWER_CONTROL:
      // Handle power control
      break;
    case protocol::CommandType::DOMAIN_BLOCK:
      if (platform_) {
        try {
          if (cmd.payload.contains("domains") &&
              cmd.payload["domains"].is_array()) {
            std::vector<std::string> domains =
                cmd.payload["domains"].get<std::vector<std::string>>();
            if (platform_->blockDomains(domains)) {
              LOG_INFO("Blocked domains successfully");
            } else {
              LOG_ERROR("Failed to block domains");
            }
          }
        } catch (const std::exception &e) {
          LOG_ERROR("Error processing DOMAIN_BLOCK: " + std::string(e.what()));
        }
      }
      break;
    case protocol::CommandType::DOMAIN_ALLOW:
      if (platform_) {
        try {
          if (cmd.payload.contains("domains") &&
              cmd.payload["domains"].is_array()) {
            std::vector<std::string> domains =
                cmd.payload["domains"].get<std::vector<std::string>>();
            if (platform_->allowDomains(domains)) {
              LOG_INFO("Allowed domains successfully");
            } else {
              LOG_ERROR("Failed to allow domains");
            }
          }
        } catch (const std::exception &e) {
          LOG_ERROR("Error processing DOMAIN_ALLOW: " + std::string(e.what()));
        }
      }
      break;
    case protocol::CommandType::APP_BLOCK:
      if (appManager_) {
        try {
          std::string app_path = cmd.payload.value("app_path", "");
          std::string app_name = cmd.payload.value("app_name", "");
          if (!app_path.empty()) {
            appManager_->addToBlacklist(app_path, app_name);
            LOG_INFO("Blocked application: " + app_path);
          }
        } catch (const std::exception &e) {
          LOG_ERROR("Error processing APP_BLOCK: " + std::string(e.what()));
        }
      }
      break;
    case protocol::CommandType::APP_ALLOW:
      if (appManager_) {
        try {
          std::string app_path = cmd.payload.value("app_path", "");
          if (!app_path.empty()) {
            appManager_->removeFromBlacklist(app_path);
            LOG_INFO("Allowed application: " + app_path);
          }
        } catch (const std::exception &e) {
          LOG_ERROR("Error processing APP_ALLOW: " + std::string(e.what()));
        }
      }
      break;
    case protocol::CommandType::APP_POLICY_SYNC:
      if (appManager_) {
        try {
          auto payload = cmd.payload;

          LOG_INFO("Synchronizing application policy from master");

          // 1. Parse and set mode
          std::string mode_str = payload.value("mode", "disabled");
          AppFilterMode mode = AppFilterMode::MODE_DISABLED;
          if (mode_str == "blacklist") {
            mode = AppFilterMode::MODE_BLACKLIST;
          } else if (mode_str == "whitelist") {
            mode = AppFilterMode::MODE_WHITELIST;
          }
          appManager_->setMode(mode);
          LOG_INFO("Application filter mode set to: " + mode_str);

          // 2. Clear existing rules and add new ones from payload
          // Note: ApplicationManager doesn't have clearRules(), so we recreate
          // it by creating a new instance or manually clearing (if we add that
          // method) For now, we'll add rules additively. In production,
          // consider adding clearRules()

          if (payload.contains("rules") && payload["rules"].is_array()) {
            for (const auto &rule_json : payload["rules"]) {
              ApplicationRule rule;
              rule.rule_id = rule_json.value("rule_id", "");
              rule.app_path = rule_json.value("app_path", "");
              rule.app_name = rule_json.value("app_name", "");
              rule.process_pattern = rule_json.value("process_pattern", "");

              // Parse action
              std::string action_str = rule_json.value("action", "block");
              rule.action = (action_str == "allow") ? RuleAction::ALLOW
                                                    : RuleAction::BLOCK;

              rule.enabled = rule_json.value("enabled", true);
              rule.created_at = rule_json.value("created_at", 0);

              appManager_->addRule(rule);
              LOG_DEBUG("Added rule: " + rule.app_name + " (" + rule.app_path +
                        ")");
            }
            LOG_INFO("Loaded " + std::to_string(payload["rules"].size()) +
                     " application rules");
          }

          // 3. Persist to disk for persistence across restarts
          if (appManager_->exportRules("app_rules.csv")) {
            LOG_INFO("Application policy saved to app_rules.csv");
          } else {
            LOG_WARNING("Failed to save application policy to file");
          }

        } catch (const std::exception &e) {
          LOG_ERROR("Failed to sync application policy: " +
                    std::string(e.what()));
        }
      }
      break;
    case protocol::CommandType::DOMAIN_POLICY_UPDATE:
      LOG_INFO("Received Domain Policy Update");
      try {
        auto payload = cmd.payload;

        // Expected payload: { "mode": "blacklist"|"whitelist", "domains":
        // ["example.com", ...] }

        // 1. Set Mode
        std::string modeStr = payload.value("mode", "blacklist");
        cms::network::FilterMode mode =
            (modeStr == "whitelist") ? cms::network::FilterMode::MODE_WHITELIST
                                     : cms::network::FilterMode::MODE_BLACKLIST;

        networkFilter_->setFilterMode(mode);

        // 2. Add Domains
        if (payload.contains("domains") && payload["domains"].is_array()) {
          for (const auto &domain : payload["domains"]) {
            if (mode == cms::network::FilterMode::MODE_BLACKLIST) {
              networkFilter_->addBlockedDomain(domain);
            } else {
              networkFilter_->addAllowedDomain(domain);
            }
          }
        }

        // 3. Apply and Save
        networkFilter_->applyRules();
        networkFilter_->saveRules();

        LOG_INFO("Domain policy applied successfully");

      } catch (const std::exception &e) {
        LOG_ERROR("Failed to apply domain policy: " + std::string(e.what()));
      }
      break;
    default:
      LOG_WARNING("Unknown command type");
      break;
    }
  }
}

void ClientService::sendScreenshot() {
  if (!connected_ || !platform_) {
    return;
  }

  try {

    // Capture screen
    cms::platform::ScreenImage image = platform_->captureScreen();
    if (image.data.empty()) {
      LOG_ERROR("Screen capture failed: empty data");
      return;
    }

    // Encode to Base64
    std::string base64Data = base64_encode(
        reinterpret_cast<unsigned char const *>(image.data.data()),
        static_cast<size_t>(image.data.size()));

    // Create payload
    nlohmann::json payload = {
        {"width", image.width},
        {"height", image.height},
        {"format", "RGBA"}, // Assuming RGBA from WindowsPlatform
        {"data", base64Data}};

    // Create message
    auto msg = protocol::Message::Create(protocol::CommandType::SCREENSHOT_DATA,
                                         config_.machine_id, "master", payload);

    // Serialize and send
    protocol::MessageSerializer serializer;
    std::string json = serializer.Serialize(msg);

    // Send with delimiter
    std::string packet = json + "\n";

    if (socket_ && socket_->Send(packet)) {
      LOG_DEBUG("Sent Screenshot Data (" + std::to_string(base64Data.size()) +
                " bytes)");
    } else {
      LOG_ERROR("Failed to send screenshot data");
    }

  } catch (const std::exception &e) {
    LOG_ERROR(std::string("Failed to send screenshot: ") + e.what());
  }
}

ClientService::~ClientService() {
  if (running_) {
    stop();
  }
  if (platform_) {
    delete platform_;
    platform_ = nullptr;
  }
  cms::Socket::Cleanup();
}

// ============================================================================
// PUBLIC INTERFACE
// ============================================================================

bool ClientService::start() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (running_) {
    LOG_WARNING("ClientService already running");
    return true;
  }

  LOG_INFO("Starting ClientService...");

  // Set start time
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  start_time_ =
      std::chrono::duration_cast<std::chrono::seconds>(duration).count();

  // Set running flag
  running_ = true;

  // Start processing thread
  processing_thread_ =
      std::make_unique<std::thread>(&ClientService::processingLoop, this);

  // Start monitor thread
  monitor_thread_ =
      std::make_unique<std::thread>(&ClientService::monitorLoop, this);

  LOG_INFO("ClientService started");
  return true;
}

bool ClientService::stop() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!running_) {
    LOG_WARNING("ClientService not running");
    return true;
  }

  LOG_INFO("Stopping ClientService...");

  // Set running flag to false
  running_ = false;

  // Notify processing thread
  cv_.notify_all();

  // Disconnect from master to interrupt blocking calls
  disconnect();

  // Wait for threads to finish
  if (processing_thread_ && processing_thread_->joinable()) {
    mutex_.unlock(); // Unlock to allow thread to finish
    processing_thread_->join();
    mutex_.lock();
  }

  if (monitor_thread_ && monitor_thread_->joinable()) {
    mutex_.unlock();
    monitor_thread_->join();
    mutex_.lock();
  }

  LOG_INFO("ClientService stopped");
  return true;
}

bool ClientService::isRunning() const { return running_.load(); }

ClientStatus ClientService::getStatus() const {
  ClientStatus status;

  status.is_connected = connected_.load();
  status.last_heartbeat = last_heartbeat_.load();
  status.service_version = CMS_VERSION_STRING;
  status.machine_id = config_.machine_id;

  // Calculate uptime
  if (running_) {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto current_time =
        std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    status.uptime = current_time - start_time_.load();
  } else {
    status.uptime = 0;
  }

  return status;
}

int ClientService::getHeartbeatInterval() const {
  return DEFAULT_HEARTBEAT_INTERVAL;
}

int ClientService::getMaxReconnectAttempts() const {
  return MAX_RECONNECT_ATTEMPTS;
}

size_t ClientService::getPendingCommandCount() const {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  return command_queue_.size();
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

void ClientService::loadConfig(const std::string &config_path) {
  try {
    std::ifstream configFile(config_path);
    if (!configFile.is_open()) {
      throw std::runtime_error("Failed to open config file: " + config_path);
    }

    nlohmann::json configJson;
    configFile >> configJson;

    // Parse configuration
    config_.master_address = configJson["master_address"].get<std::string>();
    config_.master_port = configJson["master_port"].get<int>();
    config_.machine_id = configJson["machine_id"].get<std::string>();
    config_.encryption_enabled = configJson["encryption_enabled"].get<bool>();
    config_.log_level = configJson["log_level"].get<std::string>();

    // Validate configuration
    if (config_.master_address.empty()) {
      throw std::runtime_error("Invalid config: master_address is empty");
    }
    if (config_.master_port <= 0 || config_.master_port > 65535) {
      throw std::runtime_error("Invalid config: master_port out of range");
    }
    if (config_.machine_id.empty()) {
      throw std::runtime_error("Invalid config: machine_id is empty");
    }

  } catch (const nlohmann::json::parse_error &e) {
    throw std::runtime_error(std::string("JSON parse error: ") + e.what());
  } catch (const nlohmann::json::type_error &e) {
    throw std::runtime_error(std::string("JSON type error: ") + e.what());
  }
}

void ClientService::processingLoop() {
  LOG_INFO("Processing loop started");

  int reconnectAttempts = 0;

  while (running_) {
    try {
      // Attempt to connect if not connected
      if (!connected_) {
        if (connectToMaster()) {
          reconnectAttempts = 0;
        } else {
          reconnectAttempts++;
          if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
            LOG_ERROR("Max reconnection attempts reached");
            // Continue trying, but log the issue
            reconnectAttempts = 0;
          }

          // Wait before next reconnect attempt
          std::unique_lock<std::mutex> lock(cv_mutex_);
          cv_.wait_for(lock, std::chrono::seconds(RECONNECT_DELAY),
                       [this] { return !running_; });
          if (!running_)
            break;
          continue;
        }
      }

      // Send heartbeat if needed
      auto now = std::chrono::system_clock::now();
      auto duration = now.time_since_epoch();
      auto current_time =
          std::chrono::duration_cast<std::chrono::seconds>(duration).count();

      if (current_time - last_heartbeat_.load() >= DEFAULT_HEARTBEAT_INTERVAL) {
        sendHeartbeat();
      }

      // Process incoming commands
      processCommands();

      // Sleep briefly to avoid busy-waiting
      std::unique_lock<std::mutex> lock(cv_mutex_);
      cv_.wait_for(lock, std::chrono::milliseconds(100),
                   [this] { return !running_; });

    } catch (const std::exception &e) {
      LOG_ERROR(std::string("Error in processing loop: ") + e.what());

      // Disconnect and try to reconnect
      disconnect();
      std::unique_lock<std::mutex> lock(cv_mutex_);
      cv_.wait_for(lock, std::chrono::seconds(RECONNECT_DELAY),
                   [this] { return !running_; });
    }
  }

  LOG_INFO("Processing loop stopped");
}

bool ClientService::connectToMaster() {
  LOG_INFO("Attempting to connect to master at " + config_.master_address +
           ":" + std::to_string(config_.master_port));

  try {
    socket_ = std::make_unique<cms::Socket>();
    if (!socket_->Connect(config_.master_address, config_.master_port)) {
      LOG_WARNING("Failed to connect to master");
      socket_.reset();
      connected_ = false;
      return false;
    }

    connected_ = true;
    LOG_INFO("Connected to master");

    // Perform handshake
    if (sendHello()) {
      // Start read loop
      read_thread_ =
          std::make_unique<std::thread>(&ClientService::readLoop, this);
      return true;
    }

  } catch (const std::exception &e) {
    LOG_ERROR(std::string("Connection exception: ") + e.what());
    socket_.reset();
    connected_ = false;
    return false;
  }

  return false;
}

void ClientService::disconnect() {
  if (!connected_) {
    return;
  }

  LOG_INFO("Disconnecting from master");

  if (socket_) {
    socket_->Close();
    // Do not reset socket_ here to avoid race condition with processing thread
    // socket_.reset();
  }

  connected_ = false;

  if (read_thread_ && read_thread_->joinable()) {
    if (std::this_thread::get_id() != read_thread_->get_id()) {
      read_thread_->join();
    } else {
      read_thread_->detach();
    }
  }
}

bool ClientService::sendHello() {
  if (!connected_) {
    LOG_WARNING("Cannot send HELLO: not connected");
    return false;
  }

  LOG_INFO("Sending HELLO handshake");

  try {
    // Get hostname
    char hostname[256];
#ifdef _WIN32
    DWORD size = sizeof(hostname);
    GetComputerNameA(hostname, &size);
#else
    gethostname(hostname, sizeof(hostname));
#endif

    // Create HELLO message
    nlohmann::json payload = {
        {"version", CMS_VERSION_STRING},
        {"machine_id", config_.machine_id},
        {"hostname", std::string(hostname)},
        {"capabilities",
         nlohmann::json::array(
             {"screenshot", "screen_lock", "power_control", "domain_filter"})}};

    auto helloMsg = protocol::Message::Create(
        protocol::CommandType::HELLO, config_.machine_id, "master", payload);

    // Serialize message
    protocol::MessageSerializer serializer;
    std::string json = serializer.Serialize(helloMsg);

    // Send with delimiter
    std::string packet = json + "\n";

    LOG_DEBUG("HELLO message: " + json);

    if (socket_ && socket_->Send(packet)) {
      return true;
    } else {
      LOG_ERROR("Socket send failed");
      return false;
    }

  } catch (const std::exception &e) {
    LOG_ERROR(std::string("Failed to send HELLO: ") + e.what());
    return false;
  }
}

void ClientService::sendHeartbeat() {
  if (!connected_) {
    LOG_DEBUG("Cannot send heartbeat: not connected");
    return;
  }

  LOG_DEBUG("Sending heartbeat");

  try {
    // Create PING message
    auto pingMsg = protocol::Message::Create(protocol::CommandType::PING,
                                             config_.machine_id, "master",
                                             nlohmann::json::object());

    // Serialize message
    protocol::MessageSerializer serializer;
    std::string json = serializer.Serialize(pingMsg);

    // Send with delimiter
    std::string packet = json + "\n";

    if (socket_ && socket_->Send(packet)) {
      // Success
    } else {
      LOG_ERROR("Failed to send heartbeat");
      return;
    }

    // Update last heartbeat timestamp
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    last_heartbeat_ =
        std::chrono::duration_cast<std::chrono::seconds>(duration).count();

  } catch (const std::exception &e) {
    LOG_ERROR(std::string("Failed to send heartbeat: ") + e.what());
  }
}

void ClientService::handleReconnection() {
  LOG_INFO("Handling reconnection");

  disconnect();

  // Wait before reconnecting
  std::unique_lock<std::mutex> lock(cv_mutex_);
  cv_.wait_for(lock, std::chrono::seconds(RECONNECT_DELAY),
               [this] { return !running_; });

  connectToMaster();
}

void ClientService::readLoop() {
  LOG_INFO("Read loop started");
  protocol::MessageSerializer serializer;
  std::string buffer;
  char tempBuffer[4096];

  while (connected_ && running_) {
    if (!socket_)
      break;
    int bytesRead = socket_->Receive(tempBuffer, sizeof(tempBuffer) - 1);
    if (bytesRead > 0) {
      tempBuffer[bytesRead] = '\0';
      buffer += tempBuffer;

      size_t pos;
      while ((pos = buffer.find('\n')) != std::string::npos) {
        std::string line = buffer.substr(0, pos);
        buffer.erase(0, pos + 1);

        try {
          auto msg = serializer.Deserialize(line);
          {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            command_queue_.push(msg);
          }
          LOG_INFO("Received command: " +
                   protocol::CommandTypeToString(msg.type));
        } catch (const std::exception &e) {
          LOG_ERROR("Failed to parse message: " + std::string(e.what()));
        }
      }
    } else {
      LOG_ERROR("Socket receive failed or closed");
      connected_ = false;
      cv_.notify_all(); // Wake up processing loop to reconnect
      break;
    }
  }
  LOG_INFO("Read loop stopped");
}

void ClientService::monitorLoop() {
  LOG_INFO("Application monitor loop started");
  while (running_) {
    try {
      if (platform_ && appManager_) {
        std::vector<uint32_t> pids = platform_->getRunningPids();
        for (uint32_t pid : pids) {
          // Check if allowed (this internally resolves path)
          if (!appManager_->isApplicationAllowed(pid)) {
            LOG_INFO("Blocking forbidden application (PID: " +
                     std::to_string(pid) + ")");
            appManager_->blockRunningApplication(pid);
          }
        }
      }
    } catch (const std::exception &e) {
      LOG_ERROR("Error in monitor loop: " + std::string(e.what()));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(MONITOR_INTERVAL_MS));
  }
  LOG_INFO("Application monitor loop stopped");
}

} // namespace client
} // namespace cms
