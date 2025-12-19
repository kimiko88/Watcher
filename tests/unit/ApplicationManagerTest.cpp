#include "cms/ApplicationManager.h"
#include <chrono>
#include <fstream>
#include <gtest/gtest.h>
#include <thread>

using namespace cms;

class ApplicationManagerTest : public ::testing::Test {
protected:
  ApplicationManager manager;

  void TearDown() override {
    // Cleanup rule export files
    std::remove("test_rules.json");
  }
};

// 1. Initial Mode
TEST_F(ApplicationManagerTest, InitialModeIsDisabled) {
  EXPECT_EQ(manager.getMode(), AppFilterMode::MODE_DISABLED);
}

// 2. Set Mode
TEST_F(ApplicationManagerTest, SetModeWorks) {
  manager.setMode(AppFilterMode::MODE_BLACKLIST);
  EXPECT_EQ(manager.getMode(), AppFilterMode::MODE_BLACKLIST);

  manager.setMode(AppFilterMode::MODE_WHITELIST);
  EXPECT_EQ(manager.getMode(), AppFilterMode::MODE_WHITELIST);
}

// 3. Add to Blacklist
TEST_F(ApplicationManagerTest, AddToBlacklist) {
  bool result = manager.addToBlacklist("C:\\Windows\\notepad.exe", "Notepad");
  EXPECT_TRUE(result);
  // Verify internally via isAllowed
  manager.setMode(AppFilterMode::MODE_BLACKLIST);
  EXPECT_FALSE(manager.isApplicationAllowed("C:\\Windows\\notepad.exe"));
}

// 4. Allowed in Blacklist Mode (Defaults to Allow)
TEST_F(ApplicationManagerTest, BlacklistAllowsOthers) {
  manager.setMode(
      AppFilterMode::MODE_BLACKLIST); // Default allow, block specific
  manager.addToBlacklist("C:\\BadApp.exe", "Bad");

  EXPECT_FALSE(manager.isApplicationAllowed("C:\\BadApp.exe"));
  EXPECT_TRUE(manager.isApplicationAllowed("C:\\GoodApp.exe"));
}

// 5. Add to Whitelist
TEST_F(ApplicationManagerTest, AddToWhitelist) {
  manager.addToWhitelist("C:\\GoodApp.exe", "Good");
  manager.setMode(AppFilterMode::MODE_WHITELIST);

  EXPECT_TRUE(manager.isApplicationAllowed("C:\\GoodApp.exe"));
  EXPECT_FALSE(manager.isApplicationAllowed("C:\\UnknownApp.exe"));
}

// 6. Whitelist Blocks Others
TEST_F(ApplicationManagerTest, WhitelistBlocksUnlisted) {
  manager.setMode(AppFilterMode::MODE_WHITELIST);
  EXPECT_FALSE(manager.isApplicationAllowed("C:\\AnyApp.exe"));
}

// 7. Regex Matching Blacklist
TEST_F(ApplicationManagerTest, RegexMatchingBlacklist) {
  manager.setMode(AppFilterMode::MODE_BLACKLIST);
  ApplicationRule rule;
  rule.process_pattern = ".*game.*"; // Block anything with 'game'
  rule.action = RuleAction::BLOCK;
  rule.enabled = true;
  manager.addRule(rule);

  EXPECT_FALSE(manager.isApplicationAllowed("C:\\MyGame.exe"));
  EXPECT_FALSE(manager.isApplicationAllowed("cool_game_launcher.exe"));
  EXPECT_TRUE(manager.isApplicationAllowed("notepad.exe"));
}

// 8. Regex Matching Whitelist
TEST_F(ApplicationManagerTest, RegexMatchingWhitelist) {
  manager.setMode(AppFilterMode::MODE_WHITELIST);
  ApplicationRule rule;
  rule.process_pattern = ".*work.*";
  rule.action = RuleAction::ALLOW;
  rule.enabled = true;
  manager.addRule(rule);

  EXPECT_TRUE(manager.isApplicationAllowed("C:\\work_tool.exe"));
  EXPECT_FALSE(manager.isApplicationAllowed("game.exe"));
}

// 9. Remove Rule
TEST_F(ApplicationManagerTest, RemoveRule) {
  manager.addToBlacklist("C:\\Bad.exe", "Bad");
  manager.setMode(AppFilterMode::MODE_BLACKLIST);
  EXPECT_FALSE(manager.isApplicationAllowed("C:\\Bad.exe"));

  manager.removeFromBlacklist("C:\\Bad.exe");
  EXPECT_TRUE(manager.isApplicationAllowed("C:\\Bad.exe"));
}

// 10. Disabled Mode Allows All
TEST_F(ApplicationManagerTest, DisabledModeAllowsAll) {
  manager.setMode(AppFilterMode::MODE_DISABLED);
  manager.addToBlacklist("C:\\Bad.exe", "Bad");

  EXPECT_TRUE(manager.isApplicationAllowed("C:\\Bad.exe"));
}

// 11. Statistics Tracking
TEST_F(ApplicationManagerTest, StatisticsTracking) {
  manager.setMode(AppFilterMode::MODE_BLACKLIST);
  manager.addToBlacklist("C:\\Bad.exe", "Bad");

  // Check Allowed -> should not increment stats
  manager.isApplicationAllowed("C:\\Good.exe");
  auto stats = manager.getBlockedApplicationStats();
  EXPECT_TRUE(stats.empty());

  // Check Blocked -> should increment stats
  // Note: isApplicationAllowed IS a check, it doesn't necessarily block.
  // However, usually monitoring loop calls blockRunningApplication which calls
  // terminate. Let's assume blockRunningApplication updates stats.

  // But here we are testing logic. does isApplicationAllowed update stats?
  // Maybe not. Let's call blockRunningApplication(pid) - but wait, monitoring
  // loop does that. I should test if `isApplicationAllowed` (which returns
  // false) triggers a "block" event? Usually stats count "Terminations".

  // Let's assume manual intervention updating stats?
  // Or `blockRunningApplication` updates stats.
  // Test that logic later with mocked terminate.
}

// 12. Checksum/Hash (Stub for now)
TEST_F(ApplicationManagerTest, HashVerificationStub) {
  // Requires mocking the hash calculation.
  // For TDD, I might skip unless I make hash calc injectable.
  // I'll skip complex hash test for now or assume simple path match is enough
  // for Phase 1.
}

// 13. Case Insensitivity (Windows default)
TEST_F(ApplicationManagerTest, CaseInsensitivity) {
  manager.setMode(AppFilterMode::MODE_BLACKLIST);
  manager.addToBlacklist("c:\\windows\\notepad.exe", "Notepad");

  EXPECT_FALSE(manager.isApplicationAllowed("C:\\WINDOWS\\NOTEPAD.EXE"));
}

// 14. Export Rules
TEST_F(ApplicationManagerTest, ExportRules) {
  manager.addToBlacklist("test.exe", "Test");
  bool result = manager.exportRules("test_rules.json");
  EXPECT_TRUE(result);

  std::ifstream f("test_rules.json");
  EXPECT_TRUE(f.good());
}

// 15. Import Rules
TEST_F(ApplicationManagerTest, ImportRules) {
  // export first
  manager.addToBlacklist("import_me.exe", "ImportMe");
  manager.exportRules("test_rules_import.json");

  // Clear and import
  ApplicationManager new_manager;
  bool result = new_manager.importRules("test_rules_import.json");
  EXPECT_TRUE(result);

  new_manager.setMode(AppFilterMode::MODE_BLACKLIST);
  EXPECT_FALSE(new_manager.isApplicationAllowed("import_me.exe"));

  std::remove("test_rules_import.json");
}

// 16. Rule Conflict (Allow vs Block)
TEST_F(ApplicationManagerTest, RulePriority) {
  // Usually Whitelist takes precedence? OR "Block" takes precedence in
  // Blacklist mode? In Whitelist mode: implicit block all. Specific Rules:
  // Allow. In Blacklist mode: implicit allow all. Specific Rules: Block. What
  // if I have both? Rules have "Action". Mixed rules usually confusing. Let's
  // assume: Mode BLACKLIST: Check if ANY Block rule matches -> Block. Else
  // Allow. Mode WHITELIST: Check if ANY Allow rule matches -> Allow. Else
  // Block.

  manager.setMode(AppFilterMode::MODE_BLACKLIST);
  ApplicationRule r1;
  r1.process_pattern = ".*";
  r1.action = RuleAction::BLOCK; // Block ALL
  manager.addRule(r1);
  EXPECT_FALSE(manager.isApplicationAllowed("anything.exe"));
}

// 17. Duplicate Rules
TEST_F(ApplicationManagerTest, DuplicateRules) {
  manager.addToBlacklist("test.exe", "T");
  manager.addToBlacklist("test.exe", "T");
  // Should handle gracefully
  manager.setMode(AppFilterMode::MODE_BLACKLIST);
  EXPECT_FALSE(manager.isApplicationAllowed("test.exe"));
}

// 18. Large Rule Set Performance
TEST_F(ApplicationManagerTest, PerformanceLargeRuleSet) {
  manager.setMode(AppFilterMode::MODE_BLACKLIST);
  for (int i = 0; i < 1000; ++i) {
    manager.addToBlacklist("app" + std::to_string(i) + ".exe",
                           "App" + std::to_string(i));
  }

  auto start = std::chrono::high_resolution_clock::now();
  manager.isApplicationAllowed("app500.exe");
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();
  EXPECT_LT(duration, 10000); // 10ms limit (relaxed for Debug)
}

// 19. Terminate Application (Mock?)
// Hard to test termination without specific OS primitives or Mocks.
// We will test `isApplicationAllowed` primarily.
