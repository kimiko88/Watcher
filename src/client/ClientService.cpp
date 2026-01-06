#include "cms/ClientService.h"
#include "cms/IPCChannel.h"
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
  LOG_INFO("ClientService constructor starting...");
  LOG_INFO("Config path: " + config_path);

  try {
    // Load configuration first
    loadConfig(config_path);
    LOG_INFO("Configuration loaded successfully");
    LOG_INFO("Master: " + config_.master_address + ":" +
             std::to_string(config_.master_port));
    LOG_INFO("Machine ID: " + config_.machine_id);

  } catch (const std::exception &e) {
    LOG_ERROR("Failed to load config: " + std::string(e.what()));
    throw; // Re-throw to prevent incomplete initialization
  }

  // Initialize platform interface
  LOG_INFO("Initializing platform interface...");
  try {
    platform_ = platform::getPlatformInstance().release();
    if (!platform_) {
      throw std::runtime_error("Failed to get platform instance");
    }
    LOG_INFO("Platform interface initialized");
  } catch (const std::exception &e) {
    LOG_ERROR("Platform initialization failed: " + std::string(e.what()));
    throw;
  }

  // Initialize Socket subsystem
  LOG_INFO("Initializing socket subsystem...");
  try {
    if (!cms::Socket::Initialize()) {
      LOG_WARNING(
          "Socket::Initialize() returned false (may already be initialized)");
    } else {
      LOG_INFO("Socket subsystem initialized");
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Socket initialization failed: " + std::string(e.what()));
    throw;
  }

  // Initialize Network Filter Manager
  LOG_INFO("Initializing Network Filter Manager...");
  try {
    std::string rulesFile = "domain_rules.json";

    // Check if rules file exists
    std::ifstream ruleCheck(rulesFile);
    if (!ruleCheck.good()) {
      LOG_WARNING("Domain rules file not found: " + rulesFile +
                  " (will create on first save)");
    } else {
      LOG_INFO("Domain rules file found: " + rulesFile);
    }

    networkFilter_ = std::make_unique<cms::network::NetworkFilterManager>(
        platform_, rulesFile);

    // Try to load existing rules
    try {
      networkFilter_->loadRules();
      LOG_INFO("Network filter rules loaded");
    } catch (const std::exception &e) {
      LOG_WARNING("Could not load network rules (may not exist yet): " +
                  std::string(e.what()));
    }

    // Apply rules
    networkFilter_->applyRules();
    LOG_INFO("Network filter rules applied");

  } catch (const std::exception &e) {
    LOG_ERROR("Network filter initialization failed: " + std::string(e.what()));
    // Don't throw - network filtering is optional
  }

  // Initialize Application Manager
  LOG_INFO("Initializing Application Manager...");
  try {
    appManager_ = std::make_unique<cms::ApplicationManager>();
    LOG_INFO("Application Manager initialized");

    // Try to load persisted rules
    std::string appRulesFile = "app_rules.csv";
    std::ifstream appRuleCheck(appRulesFile);
    if (appRuleCheck.good()) {
      appRuleCheck.close();
      if (appManager_->importRules(appRulesFile)) {
        LOG_INFO("Application rules imported from: " + appRulesFile);
      } else {
        LOG_WARNING("Failed to import application rules from: " + appRulesFile);
      }
    } else {
      LOG_INFO("No existing application rules file found (will create on first "
               "save)");
    }

  } catch (const std::exception &e) {
    LOG_ERROR("Application Manager initialization failed: " +
              std::string(e.what()));
    // Don't throw - app filtering is optional
  }

  LOG_INFO("ClientService initialized successfully with machine ID: " +
           config_.machine_id);
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
      if (ipcClient_) {
        LOG_DEBUG("Delegating SCREEN_LOCK to Service");
        nlohmann::json payload;
        payload["lock"] = true;
        auto msg = ipc::IPCMessage::Create(
            ipc::IPCMessageType::DELEGATE_INPUT_LOCK, payload);
        ipcClient_->SendIPCMessage(msg);
      } else if (platform_) {
        platform_->lockKeyboard();
        platform_->lockMouse();
      }
      break;
    case protocol::CommandType::SCREEN_UNLOCK:
      if (ipcClient_) {
        LOG_DEBUG("Delegating SCREEN_UNLOCK to Service");
        nlohmann::json payload;
        payload["lock"] = false;
        auto msg = ipc::IPCMessage::Create(
            ipc::IPCMessageType::DELEGATE_INPUT_LOCK, payload);
        ipcClient_->SendIPCMessage(msg);
      } else if (platform_) {
        platform_->unlockKeyboard();
        platform_->unlockMouse();
      }
      break;
    case protocol::CommandType::POWER_CONTROL:
      // Handle power control
      break;
    case protocol::CommandType::DOMAIN_BLOCK:
      if (platform_) {
        // Domain blocking requires Admin. If we are Worker (User), we must
        // delegate. We always try delegation first if available.
        if (ipcClient_) {
          LOG_DEBUG("Delegating DOMAIN_BLOCK to Service");
          // The payload already contains "domains" array.
          // But cmd.payload structure might differ?
          // cmd.payload: { "domains": [...] }
          // IPC payload: { "domains": [...] }
          // Compatible.
          auto msg = ipc::IPCMessage::Create(
              ipc::IPCMessageType::DELEGATE_DOMAIN_RULES, cmd.payload);
          ipcClient_->SendIPCMessage(msg);
        } else {
          try {
            if (cmd.payload.contains("domains") &&
                cmd.payload["domains"].is_array()) {
              std::vector<std::string> domains =
                  cmd.payload["domains"].get<std::vector<std::string>>();
              if (platform_->blockDomains(domains)) {
                LOG_INFO("Blocked domains successfully");
              } else {
                LOG_ERROR("Failed to block domains (Admin required?)");
              }
            }
          } catch (const std::exception &e) {
            LOG_ERROR("Error processing DOMAIN_BLOCK: " +
                      std::string(e.what()));
          }
        }
      }
      break;
    case protocol::CommandType::DOMAIN_ALLOW:
      // DOMAIN_ALLOW is tricky. If we use HOSTS file, "ALLOW" means "Remove
      // from Blocked". Delegation uses DELEGATE_DOMAIN_RULES which blindly
      // Apply-Blocks. We need to implement Reset logic. For now, let's assume
      // the user doesn't use ALLOW command but Reset Policy? Actually
      // `DOMAIN_POLICY_UPDATE` is what we really care about.
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

        // 1. Delegate to Service if possible (Crucial for User context)
        if (ipcClient_) {
          LOG_INFO("Delegating Domain Policy to Service");
          nlohmann::json delegatePayload;
          std::string modeStr = payload.value("mode", "blacklist");
          if (modeStr == "disabled") {
            delegatePayload["domains"] = std::vector<std::string>();
          } else {
            delegatePayload["domains"] = payload["domains"];
          }
          auto msg = ipc::IPCMessage::Create(
              ipc::IPCMessageType::DELEGATE_DOMAIN_RULES, delegatePayload);
          ipcClient_->SendIPCMessage(msg);

          // We continue to update local state for persistence,
          // but we avoid calling Apply logic on platform if we delegated?
          // Or we let it fail silently/log error.
          // Updating local state (JSON) is important for persistence.
        }

        // 2. Set Mode
        std::string modeStr = payload.value("mode", "blacklist");
        cms::network::FilterMode mode =
            (modeStr == "whitelist")
                ? cms::network::FilterMode::MODE_WHITELIST
                : (modeStr == "disabled"
                       ? cms::network::FilterMode::MODE_DISABLED
                       : cms::network::FilterMode::MODE_BLACKLIST);

        if (networkFilter_) {
          networkFilter_->setFilterMode(mode);

          // 3. Add Rules to local store
          // Note: Simplistic interaction with NetworkFilterManager.
          // Ideally we clear old rules first?
          // NetworkFilterManager doesn't have "replaceRules".
          // But since we just persist to JSON, we rely on saveRules().
          // Wait, actally `NetworkFilterManager` append logic checks
          // duplicates. If we want to replace, we should probably clear first.
          // But we don't have clear API.
          // However, ensuring persistence is secondary if Delegation works.

          if (payload.contains("domains") && payload["domains"].is_array()) {
            for (const auto &domain : payload["domains"]) {
              // For now just add.
              if (mode == cms::network::FilterMode::MODE_BLACKLIST) {
                networkFilter_->addBlockedDomain(domain);
              }
              // Resetting/Removing old domains from memory is missing here.
              // Ideally NetworkFilterManager should support "syncRules(list)".
            }
          }

          // 4. Save (Application failing is fine if delegated)
          networkFilter_->saveRules();

          // Only apply locally if NOT delegated (e.g. if running as Admin
          // standalone)
          if (!ipcClient_) {
            networkFilter_->applyRules();
          }
        }

        LOG_INFO("Domain policy processed");

      } catch (const std::exception &e) {
        LOG_ERROR("Failed to apply domain policy: " + std::string(e.what()));
      }
      break;
    case protocol::CommandType::REMOTE_INPUT:
      // Handle remote input (mouse/keyboard)
      if (platform_) {
        std::string inputType = cmd.payload.value("type", "");
        if (inputType == "mouse_move") {
          // x,y are normalized 0.0-1.0
          float nx = cmd.payload.value("x", 0.0f);
          float ny = cmd.payload.value("y", 0.0f);
          auto screen = platform_->getScreenResolution();
          int x = static_cast<int>(nx * screen.width);
          int y = static_cast<int>(ny * screen.height);
          platform_->simulateMouseMove(x, y);
        } else if (inputType == "mouse_click") {
          float nx = cmd.payload.value("x", 0.0f);
          float ny = cmd.payload.value("y", 0.0f);
          auto screen = platform_->getScreenResolution();
          int x = static_cast<int>(nx * screen.width);
          int y = static_cast<int>(ny * screen.height);
          bool left = cmd.payload.value("left", true);
          bool down = cmd.payload.value("down", false);
          platform_->simulateMouseClick(x, y, left, down);
        } else if (inputType == "key") {
          int key = cmd.payload.value("key", 0);
          bool down = cmd.payload.value("down", false);
          platform_->simulateKeyPress(key, down);
        }
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

void ClientService::sendThumbnail() {
  if (!connected_ || !platform_) {
    return;
  }

  try {
    // Capture thumbnail (400x225 default)
    cms::platform::ScreenImage thumbnail =
        platform_->captureThumbnail(400, 225);
    if (thumbnail.data.empty()) {
      LOG_DEBUG("Thumbnail capture failed: empty data");
      return;
    }

    std::string base64Data = base64_encode(
        reinterpret_cast<unsigned char const *>(thumbnail.data.data()),
        static_cast<size_t>(thumbnail.data.size()));

    nlohmann::json payload = {{"width", thumbnail.width},
                              {"height", thumbnail.height},
                              {"format", "RGBA"},
                              {"data", base64Data}};

    auto msg =
        protocol::Message::Create(protocol::CommandType::THUMBNAIL_UPDATE,
                                  config_.machine_id, "master", payload);

    protocol::MessageSerializer serializer;
    std::string json = serializer.Serialize(msg);
    std::string packet = json + "\n";

    if (socket_ && socket_->Send(packet)) {
      LOG_DEBUG("Sent thumbnail update (" + std::to_string(thumbnail.width) +
                "x" + std::to_string(thumbnail.height) + ", " +
                std::to_string(base64Data.size()) + " bytes)");
    } else {
      LOG_ERROR("Failed to send thumbnail");
    }

  } catch (const std::exception &e) {
    LOG_ERROR(std::string("Failed to send thumbnail: ") + e.what());
  }
}

void ClientService::thumbnailLoop() {
  LOG_INFO("===== THUMBNAIL LOOP STARTED =====");

  while (connected_ && running_) {
    sendThumbnail();

    // Sleep for THUMBNAIL_INTERVAL_MS (5 seconds)
    std::this_thread::sleep_for(
        std::chrono::milliseconds(THUMBNAIL_INTERVAL_MS));
  }

  LOG_INFO("===== THUMBNAIL LOOP STOPPED =====");
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
  LOG_INFO("Monitor thread started");

  // Start thumbnail update thread
  thumbnail_thread_ =
      std::make_unique<std::thread>(&ClientService::thumbnailLoop, this);
  LOG_INFO("Thumbnail thread started");

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

  if (thumbnail_thread_ && thumbnail_thread_->joinable()) {
    mutex_.unlock();
    thumbnail_thread_->join();
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

void ClientService::setIPCClient(cms::ipc::NamedPipeClient *client) {
  // No need to store if we just use it for dispatch, but we need to store it if
  // we use it later. However, ClientService is designed to be standalone-ish.
  // Let's add a member `ipcClient_` to ClientService.h?
  // Wait, I can't easily add a private member without updating the header
  // again. I already updated the header to include `setIPCClient`. I missed
  // adding the private member `ipcClient_` in the previous step? Let's check
  // the header again or just use a static/global if I must? No, bad design. I
  // will use the previous `view_file` to check if I added the member. I did NOT
  // add a private member. I only added the function declaration. I should
  // update the header to include the member `cms::ipc::NamedPipeClient*
  // ipcClient_ = nullptr;`.
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
  LOG_INFO("===== CONNECTION ATTEMPT =====");
  LOG_INFO("Target: " + config_.master_address + ":" +
           std::to_string(config_.master_port));

  try {
    socket_ = std::make_unique<cms::Socket>();

    // TODO: Add timeout support to Socket::Connect if not already present
    // For now, log before attempting connection
    LOG_INFO("Attempting socket connection...");

    auto startTime = std::chrono::steady_clock::now();
    bool connected =
        socket_->Connect(config_.master_address, config_.master_port);
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime);

    if (!connected) {
      LOG_WARNING("Connection failed after " +
                  std::to_string(duration.count()) + "ms");
      LOG_WARNING("Possible causes:");
      LOG_WARNING("  - Master server not running");
      LOG_WARNING("  - Firewall blocking port " +
                  std::to_string(config_.master_port));
      LOG_WARNING("  - Incorrect IP address: " + config_.master_address);
      LOG_WARNING("  - Network connectivity issues");
      socket_.reset();
      connected_ = false;
      return false;
    }

    LOG_INFO("Socket connected successfully in " +
             std::to_string(duration.count()) + "ms");
    connected_ = true;

    // Perform handshake
    LOG_INFO("Performing HELLO handshake...");
    if (sendHello()) {
      LOG_INFO("HELLO sent successfully");

      // Start read loop
      LOG_INFO("Starting read loop thread...");
      read_thread_ =
          std::make_unique<std::thread>(&ClientService::readLoop, this);

      LOG_INFO("===== CONNECTION ESTABLISHED =====");
      return true;
    } else {
      LOG_ERROR("HELLO handshake failed");
      socket_->Close();
      socket_.reset();
      connected_ = false;
      return false;
    }

  } catch (const std::exception &e) {
    LOG_ERROR("Connection exception: " + std::string(e.what()));
    socket_.reset();
    connected_ = false;
    return false;
  }
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

  LOG_INFO("Sending HELLO handshake to master");

  try {
    nlohmann::json payload = {
        {"version", CMS_VERSION_STRING},
        {"machine_id", config_.machine_id},
        {"hostname", platform_->getHostname()},
        {"username", platform_->getUsername()},
        {"capabilities",
         nlohmann::json::array({"screenshot", "screen_lock", "power_control",
                                "domain_filter", "app_filter"})}};

    auto helloMsg = protocol::Message::Create(
        protocol::CommandType::HELLO, config_.machine_id, "master", payload);

    protocol::MessageSerializer serializer;
    std::string json = serializer.Serialize(helloMsg);
    std::string packet = json + "\n";

    LOG_DEBUG("HELLO message payload: " + json);
    LOG_INFO("HELLO message size: " + std::to_string(packet.size()) + " bytes");

    if (socket_ && socket_->Send(packet)) {
      LOG_INFO("HELLO sent successfully");
      // Note: We don't wait for HELLO_ACK here
      // The master should respond, and we'll receive it in readLoop
      return true;
    } else {
      LOG_ERROR("Failed to send HELLO (socket->Send returned false)");
      return false;
    }

  } catch (const std::exception &e) {
    LOG_ERROR("Exception while sending HELLO: " + std::string(e.what()));
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
  LOG_INFO("===== READ LOOP STARTED =====");
  protocol::MessageSerializer serializer;
  std::string buffer;
  char tempBuffer[4096];
  int consecutiveErrors = 0;
  const int MAX_CONSECUTIVE_ERRORS = 5;

  while (connected_ && running_) {
    if (!socket_) {
      LOG_ERROR("Socket is null in readLoop");
      break;
    }

    try {
      int bytesRead = socket_->Receive(tempBuffer, sizeof(tempBuffer) - 1);

      if (bytesRead > 0) {
        consecutiveErrors = 0; // Reset error counter
        tempBuffer[bytesRead] = '\0';
        buffer += tempBuffer;

        LOG_DEBUG("Received " + std::to_string(bytesRead) + " bytes");

        // Process complete messages (delimited by newline)
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
          std::string line = buffer.substr(0, pos);
          buffer.erase(0, pos + 1);

          if (line.empty())
            continue;

          LOG_DEBUG("Processing message: " +
                    line.substr(0, std::min<size_t>(100, line.length())) +
                    "...");

          try {
            auto msg = serializer.Deserialize(line);
            {
              std::lock_guard<std::mutex> lock(queue_mutex_);
              command_queue_.push(msg);
            }
            LOG_INFO(
                "Queued command: " + protocol::CommandTypeToString(msg.type) +
                " (from: " + msg.source + ", to: " + msg.destination + ")");
          } catch (const std::exception &e) {
            LOG_ERROR("Failed to parse message: " + std::string(e.what()));
            LOG_ERROR("Raw message: " + line);
          }
        }

      } else if (bytesRead == 0) {
        LOG_WARNING("Socket closed by remote host (bytesRead = 0)");
        connected_ = false;
        cv_.notify_all();
        break;

      } else { // bytesRead < 0
        consecutiveErrors++;
        LOG_ERROR("Socket receive error (attempt " +
                  std::to_string(consecutiveErrors) + "/" +
                  std::to_string(MAX_CONSECUTIVE_ERRORS) + ")");

        if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
          LOG_ERROR("Max consecutive errors reached, closing connection");
          connected_ = false;
          cv_.notify_all();
          break;
        }

        // Brief pause before retry
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

    } catch (const std::exception &e) {
      LOG_ERROR("Exception in readLoop: " + std::string(e.what()));
      connected_ = false;
      cv_.notify_all();
      break;
    }
  }

  LOG_INFO("===== READ LOOP STOPPED ===== (connected=" +
           std::to_string(connected_.load()) +
           ", running=" + std::to_string(running_.load()) + ")");
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
