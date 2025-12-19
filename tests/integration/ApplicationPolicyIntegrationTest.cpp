#include "cms/ApplicationManager.h"
#include "cms/Protocol.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace cms;
using namespace cms::protocol;

class ApplicationPolicyIntegrationTest : public ::testing::Test {
protected:
  std::unique_ptr<ApplicationManager> manager;

  void SetUp() override { manager = std::make_unique<ApplicationManager>(); }

  void TearDown() override {
    // Cleanup
  }

  // Helper to simulate receiving APP_POLICY_SYNC and applying it
  void applyPolicyFromMessage(const Message &msg) {
    if (msg.type != CommandType::APP_POLICY_SYNC) {
      return;
    }

    // Parse mode
    std::string mode_str = msg.payload.value("mode", "disabled");
    AppFilterMode mode = AppFilterMode::MODE_DISABLED;
    if (mode_str == "blacklist") {
      mode = AppFilterMode::MODE_BLACKLIST;
    } else if (mode_str == "whitelist") {
      mode = AppFilterMode::MODE_WHITELIST;
    }

    // Reset and apply new mode
    manager = std::make_unique<ApplicationManager>();
    manager->setMode(mode);

    // Apply rules
    if (msg.payload.contains("rules") && msg.payload["rules"].is_array()) {
      for (const auto &rule_json : msg.payload["rules"]) {
        ApplicationRule rule;
        rule.rule_id = rule_json.value("rule_id", "");
        rule.app_path = rule_json.value("app_path", "");
        rule.app_name = rule_json.value("app_name", "");
        rule.process_pattern = rule_json.value("process_pattern", "");

        std::string action_str = rule_json.value("action", "block");
        rule.action =
            (action_str == "allow") ? RuleAction::ALLOW : RuleAction::BLOCK;
        rule.enabled = rule_json.value("enabled", true);
        rule.created_at = rule_json.value("created_at", 0);

        manager->addRule(rule);
      }
    }
  }
};

// Test 1: Full Workflow - Send Policy, Apply, Verify
TEST_F(ApplicationPolicyIntegrationTest, FullPolicyWorkflow) {
  nlohmann::json payload;
  payload["mode"] = "blacklist";

  nlohmann::json rules_array = nlohmann::json::array();
  nlohmann::json rule;
  rule["rule_id"] = "block_games";
  rule["app_path"] = "C:\\Games\\game.exe";
  rule["app_name"] = "GameApp";
  rule["process_pattern"] = ".*game.*";
  rule["action"] = "block";
  rule["enabled"] = true;
  rule["created_at"] = 1234567890;
  rules_array.push_back(rule);
  payload["rules"] = rules_array;

  auto msg = Message::Create(CommandType::APP_POLICY_SYNC, "master",
                             "client_001", payload);

  applyPolicyFromMessage(msg);

  EXPECT_EQ(manager->getMode(), AppFilterMode::MODE_BLACKLIST);
  EXPECT_FALSE(manager->isApplicationAllowed("C:\\Games\\game.exe"));
  EXPECT_FALSE(manager->isApplicationAllowed("my_game_launcher.exe"));
  EXPECT_TRUE(manager->isApplicationAllowed("notepad.exe"));
}

// Test 2: Whitelist Policy Integration
TEST_F(ApplicationPolicyIntegrationTest, WhitelistPolicyIntegration) {
  nlohmann::json payload;
  payload["mode"] = "whitelist";

  nlohmann::json rules_array = nlohmann::json::array();
  nlohmann::json rule;
  rule["rule_id"] = "allow_work_tools";
  rule["app_path"] = "C:\\Tools\\editor.exe";
  rule["app_name"] = "Editor";
  rule["process_pattern"] = ".*editor.*";
  rule["action"] = "allow";
  rule["enabled"] = true;
  rules_array.push_back(rule);
  payload["rules"] = rules_array;

  auto msg = Message::Create(CommandType::APP_POLICY_SYNC, "master",
                             "client_001", payload);
  applyPolicyFromMessage(msg);

  EXPECT_EQ(manager->getMode(), AppFilterMode::MODE_WHITELIST);
  EXPECT_TRUE(manager->isApplicationAllowed("C:\\Tools\\editor.exe"));
  EXPECT_FALSE(manager->isApplicationAllowed("C:\\Games\\game.exe"));
}

// Test 3: Disabled Mode Clears Restrictions
TEST_F(ApplicationPolicyIntegrationTest, DisabledModeAllowsAll) {
  // First apply blacklist
  nlohmann::json blacklist_payload;
  blacklist_payload["mode"] = "blacklist";
  nlohmann::json rules = nlohmann::json::array();
  nlohmann::json rule;
  rule["process_pattern"] = ".*";
  rule["action"] = "block";
  rule["enabled"] = true;
  rules.push_back(rule);
  blacklist_payload["rules"] = rules;

  auto blacklist_msg = Message::Create(CommandType::APP_POLICY_SYNC, "master",
                                       "client_001", blacklist_payload);
  applyPolicyFromMessage(blacklist_msg);
  EXPECT_FALSE(manager->isApplicationAllowed("anything.exe"));

  // Now disable
  nlohmann::json disabled_payload;
  disabled_payload["mode"] = "disabled";
  disabled_payload["rules"] = nlohmann::json::array();

  auto disabled_msg = Message::Create(CommandType::APP_POLICY_SYNC, "master",
                                      "client_001", disabled_payload);
  applyPolicyFromMessage(disabled_msg);

  EXPECT_EQ(manager->getMode(), AppFilterMode::MODE_DISABLED);
  EXPECT_TRUE(manager->isApplicationAllowed("anything.exe"));
}

// Test 4: Policy Update - Overwrite Previous Rules
TEST_F(ApplicationPolicyIntegrationTest, PolicyUpdate) {
  // First policy
  nlohmann::json payload1;
  payload1["mode"] = "blacklist";
  nlohmann::json rules1 = nlohmann::json::array();
  nlohmann::json rule1;
  rule1["process_pattern"] = ".*game.*";
  rule1["action"] = "block";
  rule1["enabled"] = true;
  rules1.push_back(rule1);
  payload1["rules"] = rules1;

  auto msg1 = Message::Create(CommandType::APP_POLICY_SYNC, "master",
                              "client_001", payload1);
  applyPolicyFromMessage(msg1);
  EXPECT_FALSE(manager->isApplicationAllowed("game.exe"));

  // Updated policy - different rules
  nlohmann::json payload2;
  payload2["mode"] = "blacklist";
  nlohmann::json rules2 = nlohmann::json::array();
  nlohmann::json rule2;
  rule2["process_pattern"] = ".*browser.*";
  rule2["action"] = "block";
  rule2["enabled"] = true;
  rules2.push_back(rule2);
  payload2["rules"] = rules2;

  auto msg2 = Message::Create(CommandType::APP_POLICY_SYNC, "master",
                              "client_001", payload2);
  applyPolicyFromMessage(msg2);

  // Old rule should be gone, new rule applied
  EXPECT_TRUE(manager->isApplicationAllowed("game.exe"));
  EXPECT_FALSE(manager->isApplicationAllowed("browser.exe"));
}
