#include "cms/NetworkFilterManager.h"
#include "cms/Logger.h"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace cms {
namespace network {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

NetworkFilterManager::NetworkFilterManager(platform::INetworkFilter *filter,
                                           const std::string &config_path)
    : filter_(filter), config_path_(config_path),
      mode_(FilterMode::MODE_DISABLED) {
  if (!filter_) {
    throw std::invalid_argument("Network filter interface cannot be null");
  }
}

NetworkFilterManager::~NetworkFilterManager() {
  // Optional: Restore network state on destruction?
  // Usually better to leave it as configured, or maybe fail-safe to open.
  // For now, we do nothing to persist state across service restarts.
}

// ============================================================================
// RULE MANAGEMENT
// ============================================================================

bool NetworkFilterManager::addBlockedDomain(const std::string &domain,
                                            RuleType type) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::string normDomain = normalizeDomain(domain);

  // Check duplicates
  for (const auto &rule : rules_) {
    if (rule.domain == normDomain && rule.action == Action::BLOCK) {
      return false; // Already exists
    }
  }

  DomainRule rule;
  rule.domain = normDomain;
  rule.type = type;
  rule.action = Action::BLOCK;
  rule.created_at = std::time(nullptr);

  rules_.push_back(rule);
  LOG_INFO("Added blocked domain: " + normDomain);

  // If we are in blacklist mode, apply immediately (optimization)
  // For now, we rely on explicit apply or setMode to sync.
  return true;
}

bool NetworkFilterManager::removeBlockedDomain(const std::string &domain) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::string normDomain = normalizeDomain(domain);

  auto it =
      std::remove_if(rules_.begin(), rules_.end(), [&](const DomainRule &r) {
        return r.domain == normDomain && r.action == Action::BLOCK;
      });

  if (it != rules_.end()) {
    rules_.erase(it, rules_.end());
    LOG_INFO("Removed blocked domain: " + normDomain);
    return true;
  }

  return false;
}

bool NetworkFilterManager::addAllowedDomain(const std::string &domain) {
  // For AllowList mode (Whitelist)
  std::lock_guard<std::mutex> lock(mutex_);
  std::string normDomain = normalizeDomain(domain);

  // Check duplicates
  for (const auto &rule : rules_) {
    if (rule.domain == normDomain && rule.action == Action::ALLOW) {
      return false;
    }
  }

  DomainRule rule;
  rule.domain = normDomain;
  rule.type = RuleType::EXACT; // Wildcards for allow might be complex
  rule.action = Action::ALLOW;
  rule.created_at = std::time(nullptr);

  rules_.push_back(rule);
  return true;
}

bool NetworkFilterManager::removeAllowedDomain(const std::string &domain) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::string normDomain = normalizeDomain(domain);

  auto it =
      std::remove_if(rules_.begin(), rules_.end(), [&](const DomainRule &r) {
        return r.domain == normDomain && r.action == Action::ALLOW;
      });

  if (it != rules_.end()) {
    rules_.erase(it, rules_.end());
    return true;
  }
  return false;
}

// ============================================================================
// QUERIES
// ============================================================================

bool NetworkFilterManager::isDomainBlocked(const std::string &domain) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::string normDomain = normalizeDomain(domain);

  // This logic depends on active mode.
  // If Disabled -> False
  // If Blacklist -> True if in blocklist
  // If Whitelist -> True if NOT in allowlist

  // HOWEVER, for simple checking of "Is this domain in the Block rules?",
  // the tests imply checking the rules list regardless of mode maybe?
  // Let's implement based on Rules presence first.

  for (const auto &rule : rules_) {
    if (rule.action == Action::BLOCK) {
      if (rule.type == RuleType::EXACT && rule.domain == normDomain) {
        return true;
      }
      if (rule.type == RuleType::WILDCARD) {
        // Simple wildcard: *.example.com matches www.example.com
        // Actually, let's just check if domain ends with the rule domain (minus
        // *.) This is a naive implementation
        if (rule.domain.size() > 2 && rule.domain.substr(0, 2) == "*.") {
          std::string suffix = rule.domain.substr(1); // .example.com
          if (normDomain.length() >= suffix.length()) {
            if (normDomain.compare(normDomain.length() - suffix.length(),
                                   suffix.length(), suffix) == 0) {
              return true;
            }
          }
        }
      }
    }
  }
  return false;
}

std::vector<DomainRule> NetworkFilterManager::getDomainList() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return rules_;
}

FilterStats NetworkFilterManager::getStatistics() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return stats_;
}

// ============================================================================
// MODE MANAGEMENT
// ============================================================================

bool NetworkFilterManager::setFilterMode(FilterMode mode) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mode_ == mode)
      return true;
    mode_ = mode;
    LOG_INFO("Filter mode changed");
  }
  return applyRules();
}

FilterMode NetworkFilterManager::getFilterMode() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return mode_;
}

// ============================================================================
// PERSISTENCE
// ============================================================================

bool NetworkFilterManager::saveRules() {
  std::lock_guard<std::mutex> lock(mutex_);

  json j;
  j["mode"] = static_cast<int>(mode_);
  j["rules"] = json::array();

  for (const auto &rule : rules_) {
    json r;
    r["domain"] = rule.domain;
    r["type"] = static_cast<int>(rule.type);
    r["action"] = static_cast<int>(rule.action);
    r["created_at"] = rule.created_at;
    j["rules"].push_back(r);
  }

  try {
    std::ofstream file(config_path_);
    if (!file.is_open())
      return false;
    file << j.dump(4);
    return true;
  } catch (...) {
    LOG_ERROR("Failed to save rules to " + config_path_);
    return false;
  }
}

bool NetworkFilterManager::loadRules() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!std::filesystem::exists(config_path_))
    return false;

  try {
    std::ifstream file(config_path_);
    if (!file.is_open())
      return false;

    json j;
    file >> j;

    mode_ = static_cast<FilterMode>(j["mode"]);
    rules_.clear();

    for (const auto &r : j["rules"]) {
      DomainRule rule;
      rule.domain = r["domain"];
      rule.type = static_cast<RuleType>(r["type"]);
      rule.action = static_cast<Action>(r["action"]);
      rule.created_at = r["created_at"];
      rules_.push_back(rule);
    }
    return true;
  } catch (...) {
    LOG_ERROR("Failed to load rules from " + config_path_);
    return false;
  }
}

// ============================================================================
// CORE APPLICATION
// ============================================================================

bool NetworkFilterManager::applyRules() {
  // Copy state to avoid holding lock during platform call
  std::vector<std::string> domainsToBlock;
  std::vector<std::string> domainsToAllow;
  FilterMode currentMode;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    currentMode = mode_;

    for (const auto &rule : rules_) {
      if (rule.action == Action::BLOCK) {
        domainsToBlock.push_back(rule.domain);
      } else if (rule.action == Action::ALLOW) {
        domainsToAllow.push_back(rule.domain);
      }
    }
  }

  if (currentMode == FilterMode::MODE_DISABLED) {
    // Clear all blocks? Or just do nothing?
    // Usually, disabling means clearing the existing blocks on system.
    // Assuming platform->allowDomains with empty list implies reset?
    // Or better, platform interface should have reset.
    // For now, let's try to unblock everything we tracked.
    std::vector<std::string> allBlocked;
    // Re-read blocked from rules to clear them
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (const auto &r : rules_)
        if (r.action == Action::BLOCK)
          allBlocked.push_back(r.domain);
    }
    return filter_->allowDomains(allBlocked);
  } else if (currentMode == FilterMode::MODE_BLACKLIST) {
    // In blacklist mode, we block the blocklist.
    filter_->setBlockListMode();
    return filter_->blockDomains(domainsToBlock);
  } else if (currentMode == FilterMode::MODE_WHITELIST) {
    // In whitelist mode, logic is platform specific.
    // For hosts file, this is hard.
    filter_->setAllowListMode();
    // Maybe we block everything except allowed?
    // Not implemented fully for hosts file yet.
    return false;
  }

  return true;
}

// ============================================================================
// HELPERS
// ============================================================================

std::string
NetworkFilterManager::normalizeDomain(const std::string &domain) const {
  std::string lower = domain;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  return lower;
}

void NetworkFilterManager::updatePlatformStats() {
  // Placeholder for fetching stats from platform if available
}

} // namespace network
} // namespace cms
