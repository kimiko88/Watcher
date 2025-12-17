#ifndef CMS_NETWORK_FILTER_MANAGER_H
#define CMS_NETWORK_FILTER_MANAGER_H

#include "Common.h"
#include "Platform.h"
#include <vector>
#include <string>
#include <mutex>
#include <ctime>

namespace cms {
namespace network {

// ============================================================================
// ENUMS & STRUCTS
// ============================================================================

enum class FilterMode {
    MODE_DISABLED,
    MODE_BLACKLIST, // Block listed domains
    MODE_WHITELIST  // Allow only listed domains (Not fully supported on all platforms)
};

enum class RuleType {
    EXACT,      // exact match (google.com)
    WILDCARD    // *.google.com (simulated)
};

enum class Action {
    BLOCK,
    ALLOW
};

struct DomainRule {
    std::string domain;
    RuleType type;
    Action action;
    time_t created_at;
};

struct FilterStats {
    uint64_t total_requests = 0;
    uint64_t blocked_requests = 0;
    uint64_t allowed_requests = 0;
    uint32_t unique_domains_blocked = 0;
    uint64_t uptime = 0;
};

// ============================================================================
// NETWORK FILTER MANAGER CLASS
// ============================================================================

class NetworkFilterManager {
public:
    // Constructor requires INetworkFilter interface and config path
    explicit NetworkFilterManager(platform::INetworkFilter* filter, const std::string& config_path);
    ~NetworkFilterManager();

    // Rule Management
    // For Blacklist mode: adds to blocked list
    bool addBlockedDomain(const std::string& domain, RuleType type = RuleType::EXACT);
    bool removeBlockedDomain(const std::string& domain);
    
    // For Whitelist mode (AllowList): adds to allow list
    bool addAllowedDomain(const std::string& domain);
    bool removeAllowedDomain(const std::string& domain);

    // Queries
    bool isDomainBlocked(const std::string& domain) const;
    std::vector<DomainRule> getDomainList() const;
    FilterStats getStatistics() const;

    // Mode Management
    bool setFilterMode(FilterMode mode);
    FilterMode getFilterMode() const;

    // Persistence
    bool saveRules();
    bool loadRules();

    // Core
    // Applies current rules to system via INetworkFilter
    bool applyRules();

private:
    platform::INetworkFilter* filter_;
    std::string config_path_;
    mutable std::mutex mutex_;
    
    FilterMode mode_;
    std::vector<DomainRule> rules_;
    FilterStats stats_;

    // Helpers
    std::string normalizeDomain(const std::string& domain) const;
    void updatePlatformStats();
};

} // namespace network
} // namespace cms

#endif // CMS_NETWORK_FILTER_MANAGER_H
