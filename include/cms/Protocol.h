#ifndef CMS_PROTOCOL_H
#define CMS_PROTOCOL_H

#include "Common.h"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace cms {
namespace protocol {

// Command types for classroom management
enum class CommandType {
  HELLO,                // Initial handshake between client and master
  SCREENSHOT_REQUEST,   // Request screenshot from client
  SCREENSHOT_DATA,      // Screenshot data response
  SCREEN_LOCK,          // Lock client screen
  SCREEN_UNLOCK,        // Unlock client screen
  SCREEN_BROADCAST,     // Broadcast screen to all clients
  POWER_CONTROL,        // Power management (shutdown/restart)
  DOMAIN_BLOCK,         // Block domain/URL
  DOMAIN_ALLOW,         // Allow domain/URL
  DOMAIN_POLICY_UPDATE, // Sync full domain policy
  STATUS_UPDATE,        // Status update from client
  PING,                 // Keepalive ping
  DISCONNECT,           // Disconnect notification
  APP_BLOCK,            // Block application
  APP_ALLOW,            // Allow application
  APP_POLICY_SYNC       // Sync full application policy
};

// Convert CommandType to string
inline std::string CommandTypeToString(CommandType type) {
  switch (type) {
  case CommandType::HELLO:
    return "HELLO";
  case CommandType::SCREENSHOT_REQUEST:
    return "SCREENSHOT_REQUEST";
  case CommandType::SCREENSHOT_DATA:
    return "SCREENSHOT_DATA";
  case CommandType::SCREEN_LOCK:
    return "SCREEN_LOCK";
  case CommandType::SCREEN_UNLOCK:
    return "SCREEN_UNLOCK";
  case CommandType::SCREEN_BROADCAST:
    return "SCREEN_BROADCAST";
  case CommandType::POWER_CONTROL:
    return "POWER_CONTROL";
  case CommandType::DOMAIN_BLOCK:
    return "DOMAIN_BLOCK";
  case CommandType::DOMAIN_ALLOW:
    return "DOMAIN_ALLOW";
  case CommandType::DOMAIN_POLICY_UPDATE:
    return "DOMAIN_POLICY_UPDATE";
  case CommandType::STATUS_UPDATE:
    return "STATUS_UPDATE";
  case CommandType::PING:
    return "PING";
  case CommandType::DISCONNECT:
    return "DISCONNECT";
  case CommandType::APP_BLOCK:
    return "APP_BLOCK";
  case CommandType::APP_ALLOW:
    return "APP_ALLOW";
  case CommandType::APP_POLICY_SYNC:
    return "APP_POLICY_SYNC";
  default:
    return "UNKNOWN";
  }
}

// Convert string to CommandType
inline CommandType StringToCommandType(const std::string &str) {
  if (str == "HELLO")
    return CommandType::HELLO;
  if (str == "SCREENSHOT_REQUEST")
    return CommandType::SCREENSHOT_REQUEST;
  if (str == "SCREENSHOT_DATA")
    return CommandType::SCREENSHOT_DATA;
  if (str == "SCREEN_LOCK")
    return CommandType::SCREEN_LOCK;
  if (str == "SCREEN_UNLOCK")
    return CommandType::SCREEN_UNLOCK;
  if (str == "SCREEN_BROADCAST")
    return CommandType::SCREEN_BROADCAST;
  if (str == "POWER_CONTROL")
    return CommandType::POWER_CONTROL;
  if (str == "DOMAIN_BLOCK")
    return CommandType::DOMAIN_BLOCK;
  if (str == "DOMAIN_ALLOW")
    return CommandType::DOMAIN_ALLOW;
  if (str == "DOMAIN_POLICY_UPDATE")
    return CommandType::DOMAIN_POLICY_UPDATE;
  if (str == "STATUS_UPDATE")
    return CommandType::STATUS_UPDATE;
  if (str == "PING")
    return CommandType::PING;
  if (str == "DISCONNECT")
    return CommandType::DISCONNECT;
  if (str == "APP_BLOCK")
    return CommandType::APP_BLOCK;
  if (str == "APP_ALLOW")
    return CommandType::APP_ALLOW;
  if (str == "APP_POLICY_SYNC")
    return CommandType::APP_POLICY_SYNC;

  throw std::invalid_argument("Invalid command type: " + str);
}

// Message structure
struct Message {
  // Header fields
  std::string message_id;
  CommandType type;
  int64_t timestamp;
  std::string source;
  std::string destination;

  // Payload (command-specific data)
  nlohmann::json payload;

  // Checksum for integrity
  std::string checksum;

  // Factory method to create a message with auto-generated fields
  static Message
  Create(CommandType type, const std::string &source,
         const std::string &destination,
         const nlohmann::json &payload = nlohmann::json::object());
};

// Generate a unique message ID (UUID-like)
std::string GenerateMessageId();

// Get current Unix timestamp
int64_t GetCurrentTimestamp();

// Message serializer/deserializer
class MessageSerializer {
public:
  MessageSerializer() = default;
  ~MessageSerializer() = default;

  // Serialize a message to JSON string
  // Calculates and adds checksum
  std::string Serialize(const Message &msg);

  // Deserialize JSON string to Message
  // Extracts all fields including checksum
  // Throws exception if JSON is malformed or missing required fields
  Message Deserialize(const std::string &json_str);

  // Validate message checksum
  // Returns true if checksum matches calculated value
  bool Validate(const Message &msg);

  // Calculate CRC32 checksum
  uint32_t CalculateCRC32(const std::string &data);

private:
  // Convert uint32_t to hex string
  std::string ToHexString(uint32_t value);

  // Get message content for checksum (header + payload, without checksum field)
  std::string GetChecksumContent(const Message &msg);
};

} // namespace protocol
} // namespace cms

#endif // CMS_PROTOCOL_H
