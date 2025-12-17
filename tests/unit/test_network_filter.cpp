#include "cms/NetworkFilterManager.h"
#include "cms/Platform.h"
#include <algorithm> // For std::remove
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>


// Use specific usings to avoid ambiguity, or just qualify everything.
using namespace cms::network;
// Do NOT use namespace cms::platform; creates ambiguity for FilterMode

// ============================================================================
// MOCK NETWORK FILTER
// ============================================================================

class MockNetworkFilter : public cms::platform::INetworkFilter {
public:
  bool blockDomains(const std::vector<std::string> &domains) override {
    blocked_domains_ = domains;
    return true;
  }

  bool allowDomains(const std::vector<std::string> &domains) override {
    // Remove allowed domains from blocked list
    for (const auto &domain : domains) {
      auto it =
          std::remove(blocked_domains_.begin(), blocked_domains_.end(), domain);
      if (it != blocked_domains_.end()) {
        blocked_domains_.erase(it, blocked_domains_.end());
      }
    }
    return true;
  }

  bool setAllowListMode() override {
    mode_ = "ALLOW_LIST";
    return true;
  }

  bool setBlockListMode() override {
    mode_ = "BLOCK_LIST";
    return true;
  }

  std::vector<cms::platform::FilterRule> getCurrentRules() override {
    std::vector<cms::platform::FilterRule> rules;
    for (const auto &d : blocked_domains_) {
      rules.push_back({d, true, "Mock Block"});
    }
    return rules;
  }

  // Test helpers
  std::vector<std::string> blocked_domains_;
  std::string mode_ = "NONE";
};

// ============================================================================
// TEST FIXTURE
// ============================================================================

class NetworkFilterManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    mockFilter = std::make_unique<MockNetworkFilter>();
    // Use a temp file for config
    configPath = "test_network_rules.json";
    manager =
        std::make_unique<NetworkFilterManager>(mockFilter.get(), configPath);
  }

  void TearDown() override {
    manager.reset();
    mockFilter.reset();
    // Remove temp file
    if (std::filesystem::exists(configPath)) {
      std::filesystem::remove(configPath);
    }
  }

  std::unique_ptr<MockNetworkFilter> mockFilter;
  std::unique_ptr<NetworkFilterManager> manager;
  std::string configPath;
};

// ============================================================================
// INITIALIZATION TESTS
// ============================================================================

TEST_F(NetworkFilterManagerTest, ConstructorValid) {
  EXPECT_NE(manager, nullptr);
}

TEST_F(NetworkFilterManagerTest, ConstructorUsingNullFilterThrows) {
  EXPECT_THROW(
      { NetworkFilterManager badManager(nullptr, configPath); },
      std::invalid_argument);
}

TEST_F(NetworkFilterManagerTest, InitialRulesAreEmpty) {
  EXPECT_TRUE(manager->getDomainList().empty());
}

TEST_F(NetworkFilterManagerTest, InitialModeIsDisabled) {
  // qualify FilterMode to be sure which one (cms::network::FilterMode)
  EXPECT_EQ(manager->getFilterMode(), cms::network::FilterMode::MODE_DISABLED);
}

// ============================================================================
// RULE MANAGEMENT TESTS
// ============================================================================

TEST_F(NetworkFilterManagerTest, AddBlockedDomain) {
  bool result = manager->addBlockedDomain("facebook.com");
  EXPECT_TRUE(result);

  auto list = manager->getDomainList();
  ASSERT_EQ(list.size(), 1);
  EXPECT_EQ(list[0].domain, "facebook.com");
  EXPECT_EQ(list[0].action, Action::BLOCK);
}

TEST_F(NetworkFilterManagerTest, AddDuplicateDomainFails) {
  manager->addBlockedDomain("facebook.com");
  bool result = manager->addBlockedDomain("facebook.com");

  EXPECT_FALSE(result); // Should fail/return false for duplicate
  EXPECT_EQ(manager->getDomainList().size(), 1);
}

TEST_F(NetworkFilterManagerTest, RemoveBlockedDomain) {
  manager->addBlockedDomain("twitter.com");

  bool result = manager->removeBlockedDomain("twitter.com");
  EXPECT_TRUE(result);
  EXPECT_TRUE(manager->getDomainList().empty());
}

TEST_F(NetworkFilterManagerTest, RemoveNonExistentDomainFails) {
  bool result = manager->removeBlockedDomain("ghost.com");
  EXPECT_FALSE(result);
}

TEST_F(NetworkFilterManagerTest, IsDomainBlockedCheck) {
  manager->addBlockedDomain("youtube.com");

  EXPECT_TRUE(manager->isDomainBlocked("youtube.com"));
  EXPECT_FALSE(manager->isDomainBlocked("google.com"));
}

TEST_F(NetworkFilterManagerTest, CaseInsensitiveDomainHandling) {
  manager->addBlockedDomain("InstaGram.com");

  // Should be normalized to lowercase
  auto list = manager->getDomainList();
  EXPECT_EQ(list[0].domain, "instagram.com");

  // Check match
  EXPECT_TRUE(manager->isDomainBlocked("instagram.com"));
  EXPECT_TRUE(manager->isDomainBlocked("Instagram.COM"));
}

// ============================================================================
// FILTER APPLICATION TESTS
// ============================================================================

TEST_F(NetworkFilterManagerTest, ApplyRulesBlacklist) {
  manager->addBlockedDomain("bad.com");
  manager->addBlockedDomain("evil.org");

  // Enable Blacklist mode
  manager->setFilterMode(cms::network::FilterMode::MODE_BLACKLIST);

  // Verify mock received the domains
  EXPECT_EQ(mockFilter->blocked_domains_.size(), 2);

  bool foundBad = false, foundEvil = false;
  for (const auto &d : mockFilter->blocked_domains_) {
    if (d == "bad.com")
      foundBad = true;
    if (d == "evil.org")
      foundEvil = true;
  }
  EXPECT_TRUE(foundBad);
  EXPECT_TRUE(foundEvil);
}

TEST_F(NetworkFilterManagerTest, ApplyRulesDisabledClearsFilter) {
  manager->addBlockedDomain("bad.com");
  manager->setFilterMode(cms::network::FilterMode::MODE_BLACKLIST);

  // Now disable
  manager->setFilterMode(cms::network::FilterMode::MODE_DISABLED);

  // Mock should have cleared blocked list - dependent on implementation
  // Assuming disabled mode clears the blocklist in platform
  EXPECT_TRUE(mockFilter->blocked_domains_.empty());
}

// ============================================================================
// PERSISTENCE TESTS
// ============================================================================

TEST_F(NetworkFilterManagerTest, SaveAndLoadRules) {
  manager->addBlockedDomain("persist.com");
  manager->setFilterMode(cms::network::FilterMode::MODE_BLACKLIST);

  bool saveResult = manager->saveRules();
  EXPECT_TRUE(saveResult);

  // Create new manager to load
  auto manager2 =
      std::make_unique<NetworkFilterManager>(mockFilter.get(), configPath);
  bool loadResult = manager2->loadRules();
  EXPECT_TRUE(loadResult);

  EXPECT_EQ(manager2->getFilterMode(),
            cms::network::FilterMode::MODE_BLACKLIST);
  EXPECT_TRUE(manager2->isDomainBlocked("persist.com"));
}

// ============================================================================
// WILDCARD SIMULATION TESTS
// ============================================================================

TEST_F(NetworkFilterManagerTest, WildcardSubdomainHandling) {
  manager->addBlockedDomain("*.gaming.com", RuleType::WILDCARD);

  auto list = manager->getDomainList();
  EXPECT_EQ(list[0].domain, "*.gaming.com");
  EXPECT_EQ(list[0].type, RuleType::WILDCARD);
}

// ============================================================================
// STATISTICS TESTS (Placeholder)
// ============================================================================

TEST_F(NetworkFilterManagerTest, GetStatisticsInitial) {
  auto stats = manager->getStatistics();
  EXPECT_EQ(stats.blocked_requests, 0);
}
