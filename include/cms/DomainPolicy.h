#ifndef CMS_DOMAIN_POLICY_H
#define CMS_DOMAIN_POLICY_H

#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <vector>


namespace cms {

enum class DomainFilterMode {
  WHITELIST, // Only allowed domains can be visited
  BLACKLIST  // All domains allowed except blocked ones
};

class DomainPolicy {
public:
  DomainPolicy();

  void setMode(DomainFilterMode mode);
  DomainFilterMode getMode() const;

  void addDomain(const std::string &domain);
  void removeDomain(const std::string &domain);
  bool isDomainAllowed(const std::string &domain) const;

  std::vector<std::string> getAllDomains() const;
  void clear();

  // Serialization
  nlohmann::json toJson() const;
  static DomainPolicy fromJson(const nlohmann::json &j);

private:
  DomainFilterMode mode_;
  std::set<std::string> domains_;
};

} // namespace cms

#endif // CMS_DOMAIN_POLICY_H
