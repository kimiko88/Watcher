#include "cms/IPCProtocol.h"
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>


namespace cms {
namespace ipc {

// ============================================================================
// IPCMessage Factory
// ============================================================================

IPCMessage IPCMessage::Create(IPCMessageType type,
                              const nlohmann::json &payload) {
  IPCMessage msg;

  IPCMessageSerializer serializer;
  msg.message_id = serializer.GenerateMessageId();
  msg.type = type;
  msg.timestamp = serializer.GetCurrentTimestamp();
  msg.payload = payload;

  return msg;
}

// ============================================================================
// IPCMessageSerializer Implementation
// ============================================================================

std::string IPCMessageSerializer::Serialize(const IPCMessage &msg) {
  nlohmann::json j;

  j["message_id"] = msg.message_id;
  j["type"] = IPCMessageTypeToString(msg.type);
  j["timestamp"] = msg.timestamp;
  j["payload"] = msg.payload;

  return j.dump();
}

IPCMessage IPCMessageSerializer::Deserialize(const std::string &json_str) {
  try {
    nlohmann::json j = nlohmann::json::parse(json_str);

    IPCMessage msg;
    msg.message_id = j.at("message_id").get<std::string>();
    msg.type = StringToIPCMessageType(j.at("type").get<std::string>());
    msg.timestamp = j.at("timestamp").get<int64_t>();
    msg.payload = j.at("payload");

    return msg;

  } catch (const std::exception &e) {
    throw std::runtime_error("Failed to deserialize IPC message: " +
                             std::string(e.what()));
  }
}

std::string IPCMessageSerializer::GenerateMessageId() {
  // Generate UUID-like message ID
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dis;

  uint64_t rand1 = dis(gen);
  uint64_t rand2 = dis(gen);

  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  ss << std::setw(16) << rand1;
  ss << std::setw(16) << rand2;

  return ss.str();
}

int64_t IPCMessageSerializer::GetCurrentTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
      .count();
}

} // namespace ipc
} // namespace cms
