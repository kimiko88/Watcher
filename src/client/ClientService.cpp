#include "cms/ClientService.h"
#include "cms/Logger.h"
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
      // Handle domain blocking
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

    // Serialize and send (TODO: Socket send)
    protocol::MessageSerializer serializer;
    std::string json = serializer.Serialize(msg);

    // In real impl: socket_->send(json)
    // logging for now to prove it works
    LOG_DEBUG("Generated Screenshot Data (" +
              std::to_string(base64Data.size()) + " bytes)");

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

  // Wait for thread to finish
  if (processing_thread_ && processing_thread_->joinable()) {
    mutex_.unlock(); // Unlock to allow thread to finish
    processing_thread_->join();
    mutex_.lock();
  }

  // Disconnect from master
  disconnect();

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
          std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY));
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
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

    } catch (const std::exception &e) {
      LOG_ERROR(std::string("Error in processing loop: ") + e.what());

      // Disconnect and try to reconnect
      disconnect();
      std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY));
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
    return sendHello();

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
    socket_.reset();
  }

  connected_ = false;
}

bool ClientService::sendHello() {
  if (!connected_) {
    LOG_WARNING("Cannot send HELLO: not connected");
    return false;
  }

  LOG_INFO("Sending HELLO handshake");

  try {
    // Create HELLO message
    nlohmann::json payload = {
        {"version", CMS_VERSION_STRING},
        {"machine_id", config_.machine_id},
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

    // TODO: Send via socket
    // send(socket_, json.c_str(), json.length(), 0);

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
  std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY));

  connectToMaster();
}

} // namespace client
} // namespace cms
