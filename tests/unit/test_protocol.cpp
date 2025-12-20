#include "cms/Protocol.h"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using namespace cms::protocol;

// Test fixture for Protocol tests
class ProtocolTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code
  }

  void TearDown() override {
    // Cleanup code
  }
};

// ============================================================================
// COMMAND TYPE TESTS
// ============================================================================

TEST_F(ProtocolTest, CommandTypeToString) {
  EXPECT_EQ(CommandTypeToString(CommandType::HELLO), "HELLO");
  EXPECT_EQ(CommandTypeToString(CommandType::SCREENSHOT_REQUEST),
            "SCREENSHOT_REQUEST");
  EXPECT_EQ(CommandTypeToString(CommandType::SCREENSHOT_DATA),
            "SCREENSHOT_DATA");
  EXPECT_EQ(CommandTypeToString(CommandType::SCREEN_LOCK), "SCREEN_LOCK");
  EXPECT_EQ(CommandTypeToString(CommandType::SCREEN_UNLOCK), "SCREEN_UNLOCK");
  EXPECT_EQ(CommandTypeToString(CommandType::SCREEN_BROADCAST),
            "SCREEN_BROADCAST");
  EXPECT_EQ(CommandTypeToString(CommandType::POWER_CONTROL), "POWER_CONTROL");
  EXPECT_EQ(CommandTypeToString(CommandType::DOMAIN_BLOCK), "DOMAIN_BLOCK");
  EXPECT_EQ(CommandTypeToString(CommandType::DOMAIN_ALLOW), "DOMAIN_ALLOW");
  EXPECT_EQ(CommandTypeToString(CommandType::STATUS_UPDATE), "STATUS_UPDATE");
  EXPECT_EQ(CommandTypeToString(CommandType::PING), "PING");
  EXPECT_EQ(CommandTypeToString(CommandType::DISCONNECT), "DISCONNECT");
}

TEST_F(ProtocolTest, StringToCommandType) {
  EXPECT_EQ(StringToCommandType("HELLO"), CommandType::HELLO);
  EXPECT_EQ(StringToCommandType("SCREENSHOT_REQUEST"),
            CommandType::SCREENSHOT_REQUEST);
  EXPECT_EQ(StringToCommandType("SCREENSHOT_DATA"),
            CommandType::SCREENSHOT_DATA);
  EXPECT_EQ(StringToCommandType("SCREEN_LOCK"), CommandType::SCREEN_LOCK);
  EXPECT_EQ(StringToCommandType("SCREEN_UNLOCK"), CommandType::SCREEN_UNLOCK);
  EXPECT_EQ(StringToCommandType("SCREEN_BROADCAST"),
            CommandType::SCREEN_BROADCAST);
  EXPECT_EQ(StringToCommandType("POWER_CONTROL"), CommandType::POWER_CONTROL);
  EXPECT_EQ(StringToCommandType("DOMAIN_BLOCK"), CommandType::DOMAIN_BLOCK);
  EXPECT_EQ(StringToCommandType("DOMAIN_ALLOW"), CommandType::DOMAIN_ALLOW);
  EXPECT_EQ(StringToCommandType("STATUS_UPDATE"), CommandType::STATUS_UPDATE);
  EXPECT_EQ(StringToCommandType("PING"), CommandType::PING);
  EXPECT_EQ(StringToCommandType("DISCONNECT"), CommandType::DISCONNECT);
}

TEST_F(ProtocolTest, StringToCommandTypeInvalid) {
  EXPECT_THROW(StringToCommandType("INVALID"), std::invalid_argument);
  EXPECT_THROW(StringToCommandType(""), std::invalid_argument);
  EXPECT_THROW(StringToCommandType("hello"),
               std::invalid_argument); // case sensitive
}

// ============================================================================
// MESSAGE CREATION TESTS
// ============================================================================

TEST_F(ProtocolTest, CreateMessageWithAllFields) {
  nlohmann::json payload = {{"test", "data"}};

  Message msg;
  msg.message_id = "test-id-123";
  msg.type = CommandType::HELLO;
  msg.timestamp = 1234567890;
  msg.source = "client-001";
  msg.destination = "master";
  msg.payload = payload;

  EXPECT_EQ(msg.message_id, "test-id-123");
  EXPECT_EQ(msg.type, CommandType::HELLO);
  EXPECT_EQ(msg.timestamp, 1234567890);
  EXPECT_EQ(msg.source, "client-001");
  EXPECT_EQ(msg.destination, "master");
  EXPECT_EQ(msg.payload["test"], "data");
}

TEST_F(ProtocolTest, CreateMessageFactory) {
  nlohmann::json payload = {{"version", "1.0.0"}};

  auto msg =
      Message::Create(CommandType::HELLO, "client-001", "master", payload);

  EXPECT_FALSE(msg.message_id.empty());
  EXPECT_EQ(msg.type, CommandType::HELLO);
  EXPECT_GT(msg.timestamp, 0);
  EXPECT_EQ(msg.source, "client-001");
  EXPECT_EQ(msg.destination, "master");
  EXPECT_EQ(msg.payload["version"], "1.0.0");
}

// ============================================================================
// UTILITY FUNCTION TESTS
// ============================================================================

TEST_F(ProtocolTest, GenerateMessageIdNonEmpty) {
  auto id = GenerateMessageId();
  EXPECT_FALSE(id.empty());
  EXPECT_GT(id.length(), 0);
}

TEST_F(ProtocolTest, GenerateMessageIdUnique) {
  auto id1 = GenerateMessageId();
  auto id2 = GenerateMessageId();
  EXPECT_NE(id1, id2);
}

TEST_F(ProtocolTest, GetCurrentTimestamp) {
  auto ts = GetCurrentTimestamp();
  EXPECT_GT(ts, 1700000000); // Reasonable timestamp (after 2023)
  EXPECT_LT(ts, 2000000000); // Before 2033
}

TEST_F(ProtocolTest, TimestampIncreases) {
  auto ts1 = GetCurrentTimestamp();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto ts2 = GetCurrentTimestamp();
  EXPECT_GT(ts2, ts1);
}

// ============================================================================
// CRC32 TESTS
// ============================================================================

TEST_F(ProtocolTest, CRC32Calculation) {
  MessageSerializer serializer;

  auto crc1 = serializer.CalculateCRC32("Hello, World!");
  auto crc2 = serializer.CalculateCRC32("Hello, World!");
  auto crc3 = serializer.CalculateCRC32("Different text");

  // Same input should give same CRC
  EXPECT_EQ(crc1, crc2);

  // Different input should give different CRC
  EXPECT_NE(crc1, crc3);
}

TEST_F(ProtocolTest, CRC32EmptyString) {
  MessageSerializer serializer;
  auto crc = serializer.CalculateCRC32("");
  EXPECT_EQ(crc, 0); // CRC32 of empty string is 0
}

// ============================================================================
// SERIALIZATION TESTS
// ============================================================================

TEST_F(ProtocolTest, SerializeHelloMessage) {
  MessageSerializer serializer;

  nlohmann::json payload = {{"version", "1.0.0"},
                            {"capabilities", {"screenshot", "lock"}}};

  auto msg =
      Message::Create(CommandType::HELLO, "client-001", "master", payload);
  auto json_str = serializer.Serialize(msg);

  EXPECT_FALSE(json_str.empty());

  // Parse the JSON to verify structure
  auto parsed = nlohmann::json::parse(json_str);
  EXPECT_TRUE(parsed.contains("header"));
  EXPECT_TRUE(parsed.contains("payload"));
  EXPECT_TRUE(parsed.contains("checksum"));
}

TEST_F(ProtocolTest, SerializeAllCommandTypes) {
  MessageSerializer serializer;
  nlohmann::json payload = {{"test", "data"}};

  std::vector<CommandType> types = {CommandType::HELLO,
                                    CommandType::SCREENSHOT_REQUEST,
                                    CommandType::SCREENSHOT_DATA,
                                    CommandType::SCREEN_LOCK,
                                    CommandType::SCREEN_UNLOCK,
                                    CommandType::SCREEN_BROADCAST,
                                    CommandType::POWER_CONTROL,
                                    CommandType::DOMAIN_BLOCK,
                                    CommandType::DOMAIN_ALLOW,
                                    CommandType::STATUS_UPDATE,
                                    CommandType::PING,
                                    CommandType::DISCONNECT};

  for (auto type : types) {
    auto msg = Message::Create(type, "source", "dest", payload);
    auto json_str = serializer.Serialize(msg);

    EXPECT_FALSE(json_str.empty());
    auto parsed = nlohmann::json::parse(json_str);
    EXPECT_EQ(parsed["header"]["type"], CommandTypeToString(type));
  }
}

TEST_F(ProtocolTest, SerializeVerifyJsonStructure) {
  MessageSerializer serializer;

  nlohmann::json payload = {{"key", "value"}};
  auto msg = Message::Create(CommandType::PING, "client", "master", payload);
  auto json_str = serializer.Serialize(msg);

  auto parsed = nlohmann::json::parse(json_str);

  // Verify header structure
  EXPECT_TRUE(parsed["header"].contains("message_id"));
  EXPECT_TRUE(parsed["header"].contains("type"));
  EXPECT_TRUE(parsed["header"].contains("timestamp"));
  EXPECT_TRUE(parsed["header"].contains("source"));
  EXPECT_TRUE(parsed["header"].contains("destination"));

  // Verify payload
  EXPECT_EQ(parsed["payload"]["key"], "value");

  // Verify checksum
  EXPECT_TRUE(parsed.contains("checksum"));
  EXPECT_TRUE(parsed["checksum"].is_string());
}

// ============================================================================
// DESERIALIZATION TESTS
// ============================================================================

TEST_F(ProtocolTest, DeserializeValidJson) {
  MessageSerializer serializer;

  std::string json_str = R"({
        "header": {
            "message_id": "test-123",
            "type": "HELLO",
            "timestamp": 1234567890,
            "source": "client-001",
            "destination": "master"
        },
        "payload": {
            "version": "1.0.0"
        },
        "checksum": "00000000"
    })";

  // This should not throw even if checksum is wrong (deserialize just parses)
  EXPECT_NO_THROW({
    auto msg = serializer.Deserialize(json_str);
    EXPECT_EQ(msg.message_id, "test-123");
    EXPECT_EQ(msg.type, CommandType::HELLO);
    EXPECT_EQ(msg.timestamp, 1234567890);
    EXPECT_EQ(msg.source, "client-001");
    EXPECT_EQ(msg.destination, "master");
    EXPECT_EQ(msg.payload["version"], "1.0.0");
  });
}

TEST_F(ProtocolTest, DeserializeExtractsChecksum) {
  MessageSerializer serializer;

  std::string json_str = R"({
        "header": {
            "message_id": "test",
            "type": "PING",
            "timestamp": 123,
            "source": "a",
            "destination": "b"
        },
        "payload": {},
        "checksum": "ABCD1234"
    })";

  auto msg = serializer.Deserialize(json_str);
  EXPECT_EQ(msg.checksum, "ABCD1234");
}

// ============================================================================
// SERIALIZATION/DESERIALIZATION IDEMPOTENCY TESTS
// ============================================================================

TEST_F(ProtocolTest, SerializeDeserializeIdempotency) {
  MessageSerializer serializer;

  nlohmann::json payload = {{"quality", 80}, {"format", "jpeg"}};

  auto original = Message::Create(CommandType::SCREENSHOT_REQUEST, "master",
                                  "client-001", payload);

  // Serialize
  auto json_str = serializer.Serialize(original);

  // Deserialize
  auto deserialized = serializer.Deserialize(json_str);

  // Compare (excluding checksum which is calculated)
  EXPECT_EQ(deserialized.message_id, original.message_id);
  EXPECT_EQ(deserialized.type, original.type);
  EXPECT_EQ(deserialized.timestamp, original.timestamp);
  EXPECT_EQ(deserialized.source, original.source);
  EXPECT_EQ(deserialized.destination, original.destination);
  EXPECT_EQ(deserialized.payload, original.payload);
}

TEST_F(ProtocolTest, IdempotencyAllCommandTypes) {
  MessageSerializer serializer;

  std::vector<CommandType> types = {CommandType::HELLO, CommandType::PING,
                                    CommandType::DISCONNECT};

  for (auto type : types) {
    nlohmann::json payload = {{"test", type}};
    auto original = Message::Create(type, "src", "dst", payload);

    auto json_str = serializer.Serialize(original);
    auto deserialized = serializer.Deserialize(json_str);

    EXPECT_EQ(deserialized.type, original.type);
    EXPECT_EQ(deserialized.payload, original.payload);
  }
}

// ============================================================================
// CHECKSUM VALIDATION TESTS
// ============================================================================

TEST_F(ProtocolTest, ValidateValidChecksum) {
  MessageSerializer serializer;

  nlohmann::json payload = {{"test", "data"}};
  auto msg = Message::Create(CommandType::HELLO, "client", "master", payload);

  // Serialize (this calculates checksum)
  auto json_str = serializer.Serialize(msg);

  // Deserialize (this loads the message with checksum)
  auto loaded_msg = serializer.Deserialize(json_str);

  // Validate should return true for valid checksum
  EXPECT_TRUE(serializer.Validate(loaded_msg));
}

TEST_F(ProtocolTest, ValidateInvalidChecksum) {
  MessageSerializer serializer;

  nlohmann::json payload = {{"test", "data"}};
  auto msg = Message::Create(CommandType::HELLO, "client", "master", payload);

  // Serialize
  auto json_str = serializer.Serialize(msg);

  // Deserialize
  auto loaded_msg = serializer.Deserialize(json_str);

  // Tamper with checksum
  loaded_msg.checksum = "FFFFFFFF";

  // Validate should return false
  EXPECT_FALSE(serializer.Validate(loaded_msg));
}

TEST_F(ProtocolTest, ValidateDetectsPayloadModification) {
  MessageSerializer serializer;

  nlohmann::json payload = {{"value", 100}};
  auto msg =
      Message::Create(CommandType::STATUS_UPDATE, "client", "master", payload);

  auto json_str = serializer.Serialize(msg);
  auto loaded_msg = serializer.Deserialize(json_str);

  // Modify payload
  loaded_msg.payload["value"] = 200;

  // Validation should fail
  EXPECT_FALSE(serializer.Validate(loaded_msg));
}

TEST_F(ProtocolTest, ValidateDetectsHeaderModification) {
  MessageSerializer serializer;

  nlohmann::json payload = {{"test", "data"}};
  auto msg = Message::Create(CommandType::PING, "client", "master", payload);

  auto json_str = serializer.Serialize(msg);
  auto loaded_msg = serializer.Deserialize(json_str);

  // Modify header
  loaded_msg.source = "attacker";

  // Validation should fail
  EXPECT_FALSE(serializer.Validate(loaded_msg));
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

TEST_F(ProtocolTest, DeserializeMalformedJson) {
  MessageSerializer serializer;

  std::string malformed = "{this is not valid json}";

  EXPECT_THROW(serializer.Deserialize(malformed), std::exception);
}

TEST_F(ProtocolTest, DeserializeMissingMessageId) {
  MessageSerializer serializer;

  std::string json_str = R"({
        "header": {
            "type": "HELLO",
            "timestamp": 123,
            "source": "a",
            "destination": "b"
        },
        "payload": {},
        "checksum": "00000000"
    })";

  EXPECT_THROW(serializer.Deserialize(json_str), std::exception);
}

TEST_F(ProtocolTest, DeserializeMissingType) {
  MessageSerializer serializer;

  std::string json_str = R"({
        "header": {
            "message_id": "test",
            "timestamp": 123,
            "source": "a",
            "destination": "b"
        },
        "payload": {},
        "checksum": "00000000"
    })";

  EXPECT_THROW(serializer.Deserialize(json_str), std::exception);
}

TEST_F(ProtocolTest, DeserializeMissingTimestamp) {
  MessageSerializer serializer;

  std::string json_str = R"({
        "header": {
            "message_id": "test",
            "type": "PING",
            "source": "a",
            "destination": "b"
        },
        "payload": {},
        "checksum": "00000000"
    })";

  EXPECT_THROW(serializer.Deserialize(json_str), std::exception);
}

TEST_F(ProtocolTest, DeserializeMissingSource) {
  MessageSerializer serializer;

  std::string json_str = R"({
        "header": {
            "message_id": "test",
            "type": "PING",
            "timestamp": 123,
            "destination": "b"
        },
        "payload": {},
        "checksum": "00000000"
    })";

  EXPECT_THROW(serializer.Deserialize(json_str), std::exception);
}

TEST_F(ProtocolTest, DeserializeMissingDestination) {
  MessageSerializer serializer;

  std::string json_str = R"({
        "header": {
            "message_id": "test",
            "type": "PING",
            "timestamp": 123,
            "source": "a"
        },
        "payload": {},
        "checksum": "00000000"
    })";

  EXPECT_THROW(serializer.Deserialize(json_str), std::exception);
}

TEST_F(ProtocolTest, DeserializeMissingChecksum) {
  MessageSerializer serializer;

  std::string json_str = R"({
        "header": {
            "message_id": "test",
            "type": "PING",
            "timestamp": 123,
            "source": "a",
            "destination": "b"
        },
        "payload": {}
    })";

  EXPECT_THROW(serializer.Deserialize(json_str), std::exception);
}

TEST_F(ProtocolTest, DeserializeInvalidCommandType) {
  MessageSerializer serializer;

  std::string json_str = R"({
        "header": {
            "message_id": "test",
            "type": "INVALID_COMMAND",
            "timestamp": 123,
            "source": "a",
            "destination": "b"
        },
        "payload": {},
        "checksum": "00000000"
    })";

  EXPECT_THROW(serializer.Deserialize(json_str), std::invalid_argument);
}

// ============================================================================
// REALISTIC SCENARIO TESTS
// ============================================================================

TEST_F(ProtocolTest, RealisticHelloHandshake) {
  MessageSerializer serializer;

  // Client creates HELLO message
  nlohmann::json hello_payload = {
      {"version", "1.0.0"},
      {"capabilities", {"screenshot", "screen_lock", "domain_block"}},
      {"os", "Windows"}};

  auto hello_msg = Message::Create(CommandType::HELLO, "client-001", "master",
                                   hello_payload);
  auto hello_json = serializer.Serialize(hello_msg);

  // Master receives and deserializes
  auto received_hello = serializer.Deserialize(hello_json);

  // Master validates
  EXPECT_TRUE(serializer.Validate(received_hello));

  // Master checks capabilities
  EXPECT_EQ(received_hello.payload["version"], "1.0.0");
  EXPECT_EQ(received_hello.payload["capabilities"].size(), 3);
}

TEST_F(ProtocolTest, RealisticScreenshotFlow) {
  MessageSerializer serializer;

  // Master requests screenshot
  nlohmann::json request_payload = {
      {"quality", 80}, {"format", "jpeg"}, {"width", 1920}, {"height", 1080}};

  auto request = Message::Create(CommandType::SCREENSHOT_REQUEST, "master",
                                 "client-001", request_payload);
  auto request_json = serializer.Serialize(request);

  // Client receives
  auto received_request = serializer.Deserialize(request_json);
  EXPECT_TRUE(serializer.Validate(received_request));

  // Client sends screenshot
  nlohmann::json data_payload = {{"data", "base64_encoded_image_data_here"},
                                 {"width", 1920},
                                 {"height", 1080},
                                 {"format", "jpeg"}};

  auto response = Message::Create(CommandType::SCREENSHOT_DATA, "client-001",
                                  "master", data_payload);
  auto response_json = serializer.Serialize(response);

  // Master receives
  auto received_response = serializer.Deserialize(response_json);
  EXPECT_TRUE(serializer.Validate(received_response));
  EXPECT_EQ(received_response.payload["width"], 1920);
}

TEST_F(ProtocolTest, RealisticDomainBlockCommand) {
  MessageSerializer serializer;

  nlohmann::json block_payload = {{"domain", "example.com"},
                                  {"reason", "inappropriate content"},
                                  {"permanent", false}};

  auto msg = Message::Create(CommandType::DOMAIN_BLOCK, "master", "client-001",
                             block_payload);
  auto json_str = serializer.Serialize(msg);

  auto received = serializer.Deserialize(json_str);

  EXPECT_TRUE(serializer.Validate(received));
  EXPECT_EQ(received.type, CommandType::DOMAIN_BLOCK);
  EXPECT_EQ(received.payload["domain"], "example.com");
  EXPECT_EQ(received.payload["permanent"], false);
}

TEST_F(ProtocolTest, RealisticStatusUpdate) {
  MessageSerializer serializer;

  nlohmann::json status_payload = {
      {"cpu_usage", 45.2},
      {"memory_usage", 60.5},
      {"disk_usage", 75.0},
      {"active_apps", {"chrome.exe", "notepad.exe"}},
      {"status", "active"}};

  auto msg = Message::Create(CommandType::STATUS_UPDATE, "client-001", "master",
                             status_payload);
  auto json_str = serializer.Serialize(msg);

  auto received = serializer.Deserialize(json_str);

  EXPECT_TRUE(serializer.Validate(received));
  EXPECT_DOUBLE_EQ(received.payload["cpu_usage"], 45.2);
  EXPECT_EQ(received.payload["active_apps"].size(), 2);
}
