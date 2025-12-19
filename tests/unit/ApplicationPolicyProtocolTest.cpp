#include "cms/Protocol.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace cms::protocol;

class ApplicationPolicyProtocolTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup test data
  }
};

// Test 1: APP_POLICY_SYNC Message Creation
TEST_F(ApplicationPolicyProtocolTest, CreateAppPolicySyncMessage) {
  nlohmann::json payload;
  payload["mode"] = "blacklist";

  nlohmann::json rules_array = nlohmann::json::array();
  nlohmann::json rule;
  rule["rule_id"] = "rule_001";
  rule["app_path"] = "C:\\\\Games\\\\game.exe";
  rule["app_name"] = "GameApp";
  rule["process_pattern"] = ".*game.*";
  rule["action"] = "block";
  rule["enabled"] = true;
  rule["created_at"] = 1234567890;
  rules_array.push_back(rule);
  payload["rules"] = rules_array;

  auto msg = Message::Create(CommandType::APP_POLICY_SYNC, "master",
                             "client_001", payload);

  EXPECT_EQ(msg.type, CommandType::APP_POLICY_SYNC);
  EXPECT_EQ(msg.source, "master");
  EXPECT_EQ(msg.destination, "client_001");
  EXPECT_FALSE(msg.payload.empty());
  EXPECT_EQ(msg.payload["mode"], "blacklist");
  EXPECT_TRUE(msg.payload.contains("rules"));
  EXPECT_TRUE(msg.payload["rules"].is_array());
  EXPECT_EQ(msg.payload["rules"].size(), 1);
}

// Test 2: APP_POLICY_SYNC Serialization
TEST_F(ApplicationPolicyProtocolTest, SerializeAppPolicySyncMessage) {
  nlohmann::json payload;
  payload["mode"] = "whitelist";
  payload["rules"] = nlohmann::json::array();

  auto msg = Message::Create(CommandType::APP_POLICY_SYNC, "master",
                             "client_001", payload);

  MessageSerializer serializer;
  std::string json_str = serializer.Serialize(msg);

  EXPECT_FALSE(json_str.empty());
  // Parse back and verify
  auto parsed = nlohmann::json::parse(json_str);
  EXPECT_EQ(parsed["type"], "APP_POLICY_SYNC");
  EXPECT_EQ(parsed["source"], "master");
}

// Test 3: APP_POLICY_SYNC Deserialization
TEST_F(ApplicationPolicyProtocolTest, DeserializeAppPolicySyncMessage) {
  std::string json_str = R"({
    "type": "APP_POLICY_SYNC",
    "source": "master",
    "destination": "client_001",
    "timestamp": 1234567890,
    "payload": {
      "mode": "blacklist",
      "rules": [
        {
          "rule_id": "rule_001",
          "app_path": "C:\\Games\\game.exe",
          "app_name": "GameApp",
          "process_pattern": ".*game.*",
          "action": "block",
          "enabled": true,
          "created_at": 1234567890
        }
      ]
    }
  })";

  MessageSerializer serializer;
  auto msg = serializer.Deserialize(json_str);

  EXPECT_EQ(msg.type, CommandType::APP_POLICY_SYNC);
  EXPECT_EQ(msg.payload["mode"], "blacklist");
  EXPECT_EQ(msg.payload["rules"][0]["app_name"], "GameApp");
}

// Test 4: Multiple Rules in Payload
TEST_F(ApplicationPolicyProtocolTest, MultipleRulesInPayload) {
  nlohmann::json payload;
  payload["mode"] = "blacklist";

  nlohmann::json rules_array = nlohmann::json::array();
  for (int i = 0; i < 5; ++i) {
    nlohmann::json rule;
    rule["rule_id"] = "rule_" + std::to_string(i);
    rule["app_name"] = "App" + std::to_string(i);
    rule["app_path"] = "C:\\\\Apps\\\\app" + std::to_string(i) + ".exe";
    rule["process_pattern"] = ".*app" + std::to_string(i) + ".*";
    rule["action"] = "block";
    rule["enabled"] = true;
    rules_array.push_back(rule);
  }
  payload["rules"] = rules_array;

  auto msg = Message::Create(CommandType::APP_POLICY_SYNC, "master",
                             "client_001", payload);

  EXPECT_EQ(msg.payload["rules"].size(), 5);
  EXPECT_EQ(msg.payload["rules"][2]["app_name"], "App2");
}

// Test 5: Disabled Mode Payload
TEST_F(ApplicationPolicyProtocolTest, DisabledModePayload) {
  nlohmann::json payload;
  payload["mode"] = "disabled";
  payload["rules"] = nlohmann::json::array();

  auto msg = Message::Create(CommandType::APP_POLICY_SYNC, "master",
                             "client_001", payload);

  EXPECT_EQ(msg.payload["mode"], "disabled");
  EXPECT_TRUE(msg.payload["rules"].is_array());
  EXPECT_EQ(msg.payload["rules"].size(), 0);
}

// Test 6: Whitelist Mode with Allow Rules
TEST_F(ApplicationPolicyProtocolTest, WhitelistModePayload) {
  nlohmann::json payload;
  payload["mode"] = "whitelist";

  nlohmann::json rules_array = nlohmann::json::array();
  nlohmann::json rule;
  rule["rule_id"] = "allow_001";
  rule["app_path"] = "C:\\\\Tools\\\\notepad.exe";
  rule["app_name"] = "Notepad";
  rule["process_pattern"] = ".*notepad.*";
  rule["action"] = "allow";
  rule["enabled"] = true;
  rules_array.push_back(rule);
  payload["rules"] = rules_array;

  auto msg = Message::Create(CommandType::APP_POLICY_SYNC, "master",
                             "client_001", payload);

  EXPECT_EQ(msg.payload["mode"], "whitelist");
  EXPECT_EQ(msg.payload["rules"][0]["action"], "allow");
}

// Test 7: Invalid JSON Handling
TEST_F(ApplicationPolicyProtocolTest, InvalidJsonHandling) {
  std::string invalid_json = "{ invalid json }";

  MessageSerializer serializer;
  EXPECT_THROW(serializer.Deserialize(invalid_json), std::exception);
}

// Test 8: Missing Required Fields
TEST_F(ApplicationPolicyProtocolTest, MissingModeField) {
  std::string json_str = R"({
    "type": "APP_POLICY_SYNC",
    "source": "master",
    "destination": "client_001",
    "timestamp": 1234567890,
    "payload": {
      "rules": []
    }
  })";

  MessageSerializer serializer;
  auto msg = serializer.Deserialize(json_str);

  // Should deserialize but payload won't have "mode"
  EXPECT_FALSE(msg.payload.contains("mode"));
  EXPECT_TRUE(msg.payload.contains("rules"));
}

// Test 9: CommandType String Conversion
TEST_F(ApplicationPolicyProtocolTest, CommandTypeToString) {
  std::string type_str = CommandTypeToString(CommandType::APP_POLICY_SYNC);
  EXPECT_EQ(type_str, "APP_POLICY_SYNC");
}

// Test 10: Empty Rules Array
TEST_F(ApplicationPolicyProtocolTest, EmptyRulesArray) {
  nlohmann::json payload;
  payload["mode"] = "blacklist";
  payload["rules"] = nlohmann::json::array();

  auto msg = Message::Create(CommandType::APP_POLICY_SYNC, "master",
                             "client_001", payload);

  EXPECT_TRUE(msg.payload["rules"].is_array());
  EXPECT_EQ(msg.payload["rules"].size(), 0);
}
