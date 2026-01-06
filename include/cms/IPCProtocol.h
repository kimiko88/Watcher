#ifndef CMS_IPC_PROTOCOL_H
#define CMS_IPC_PROTOCOL_H

#include "Common.h"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

namespace cms {
namespace ipc {

// IPC Message Types
enum class IPCMessageType {
  // Worker/GUI → Service
  PROCESS_READY,        // Process has started and is ready
  PROCESS_STATUS,       // Heartbeat and status update
  PROCESS_SHUTDOWN_ACK, // Acknowledge shutdown request

  // Service → Worker/GUI
  SERVICE_CONFIG,   // Configuration data from service
  SERVICE_RESTART,  // Request process restart
  SERVICE_SHUTDOWN, // Request graceful shutdown

  // Master GUI ↔ Master Service
  GUI_READY,       // GUI connected to service
  SERVER_STATE,    // Server state (clients, config)
  SERVER_EVENT,    // Async events (new client, screenshot)
  EXECUTE_COMMAND, // Command from GUI
  COMMAND_RESULT,  // Result of executed command

  // Generic
  PING,           // Keepalive
  PONG,           // Keepalive response
  ERROR_RESPONSE, // Error response

  // Worker → Service Delegation
  DELEGATE_DOMAIN_RULES, // Request service to apply domain rules (requires
                         // Admin)
  DELEGATE_INPUT_LOCK    // Request service to lock input (requires Admin)
};

// Convert IPCMessageType to string
inline std::string IPCMessageTypeToString(IPCMessageType type) {
  switch (type) {
  case IPCMessageType::PROCESS_READY:
    return "PROCESS_READY";
  case IPCMessageType::PROCESS_STATUS:
    return "PROCESS_STATUS";
  case IPCMessageType::PROCESS_SHUTDOWN_ACK:
    return "PROCESS_SHUTDOWN_ACK";
  case IPCMessageType::SERVICE_CONFIG:
    return "SERVICE_CONFIG";
  case IPCMessageType::SERVICE_RESTART:
    return "SERVICE_RESTART";
  case IPCMessageType::SERVICE_SHUTDOWN:
    return "SERVICE_SHUTDOWN";
  case IPCMessageType::GUI_READY:
    return "GUI_READY";
  case IPCMessageType::SERVER_STATE:
    return "SERVER_STATE";
  case IPCMessageType::SERVER_EVENT:
    return "SERVER_EVENT";
  case IPCMessageType::EXECUTE_COMMAND:
    return "EXECUTE_COMMAND";
  case IPCMessageType::COMMAND_RESULT:
    return "COMMAND_RESULT";
  case IPCMessageType::PING:
    return "PING";
  case IPCMessageType::PONG:
    return "PONG";
  case IPCMessageType::ERROR_RESPONSE:
    return "ERROR_RESPONSE";
  case IPCMessageType::DELEGATE_DOMAIN_RULES:
    return "DELEGATE_DOMAIN_RULES";
  case IPCMessageType::DELEGATE_INPUT_LOCK:
    return "DELEGATE_INPUT_LOCK";
  default:
    return "UNKNOWN";
  }
}

// Convert string to IPCMessageType
inline IPCMessageType StringToIPCMessageType(const std::string &str) {
  if (str == "PROCESS_READY")
    return IPCMessageType::PROCESS_READY;
  if (str == "PROCESS_STATUS")
    return IPCMessageType::PROCESS_STATUS;
  if (str == "PROCESS_SHUTDOWN_ACK")
    return IPCMessageType::PROCESS_SHUTDOWN_ACK;
  if (str == "SERVICE_CONFIG")
    return IPCMessageType::SERVICE_CONFIG;
  if (str == "SERVICE_RESTART")
    return IPCMessageType::SERVICE_RESTART;
  if (str == "SERVICE_SHUTDOWN")
    return IPCMessageType::SERVICE_SHUTDOWN;
  if (str == "GUI_READY")
    return IPCMessageType::GUI_READY;
  if (str == "SERVER_STATE")
    return IPCMessageType::SERVER_STATE;
  if (str == "SERVER_EVENT")
    return IPCMessageType::SERVER_EVENT;
  if (str == "EXECUTE_COMMAND")
    return IPCMessageType::EXECUTE_COMMAND;
  if (str == "COMMAND_RESULT")
    return IPCMessageType::COMMAND_RESULT;
  if (str == "PING")
    return IPCMessageType::PING;
  if (str == "PONG")
    return IPCMessageType::PONG;
  if (str == "ERROR_RESPONSE")
    return IPCMessageType::ERROR_RESPONSE;
  if (str == "DELEGATE_DOMAIN_RULES")
    return IPCMessageType::DELEGATE_DOMAIN_RULES;
  if (str == "DELEGATE_INPUT_LOCK")
    return IPCMessageType::DELEGATE_INPUT_LOCK;
  throw std::invalid_argument("Invalid IPC message type: " + str);
}

// IPC Message Structure
struct IPCMessage {
  std::string message_id; // Unique message ID
  IPCMessageType type;    // Message type
  int64_t timestamp;      // Unix timestamp
  nlohmann::json payload; // Message payload (flexible JSON)

  // Factory method to create message
  static IPCMessage
  Create(IPCMessageType type,
         const nlohmann::json &payload = nlohmann::json::object());
};

// IPC Message Serializer
class IPCMessageSerializer {
public:
  IPCMessageSerializer() = default;
  ~IPCMessageSerializer() = default;

  // Serialize message to JSON string
  std::string Serialize(const IPCMessage &msg);

  // Deserialize JSON string to message
  IPCMessage Deserialize(const std::string &json_str);

  // Generate unique message ID (static helper)
  static std::string GenerateMessageId();

  // Get current timestamp (static helper)
  static int64_t GetCurrentTimestamp();
};

// Named Pipe Names
namespace PipeNames {
constexpr const char *CLIENT_SERVICE = "\\\\.\\pipe\\WatcherClientService";
constexpr const char *MASTER_SERVICE = "\\\\.\\pipe\\WatcherMasterService";
} // namespace PipeNames

// IPC Constants
namespace IPCConstants {
constexpr size_t BUFFER_SIZE = 65536;            // 64KB buffer
constexpr uint32_t CONNECT_TIMEOUT_MS = 5000;    // 5 seconds
constexpr uint32_t READ_TIMEOUT_MS = 10000;      // 10 seconds
constexpr uint32_t HEARTBEAT_INTERVAL_MS = 5000; // 5 seconds
} // namespace IPCConstants

} // namespace ipc
} // namespace cms

#endif // CMS_IPC_PROTOCOL_H
