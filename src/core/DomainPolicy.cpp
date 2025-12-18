#include "cms/DomainPolicy.h"
#include <algorithm>

namespace cms {

DomainPolicy::DomainPolicy() : mode_(DomainFilterMode::BLACKLIST) {}

void DomainPolicy::setMode(DomainFilterMode mode) { mode_ = mode; }

DomainFilterMode DomainPolicy::getMode() const { return mode_; }

void DomainPolicy::addDomain(const std::string &domain) {
  if (!domain.empty()) {
    domains_.insert(domain);
  }
}

void DomainPolicy::removeDomain(const std::string &domain) {
  domains_.erase(domain);
}

bool DomainPolicy::isDomainAllowed(const std::string &domain) const {
  bool found = domains_.find(domain) != domains_.end();

  if (mode_ == DomainFilterMode::WHITELIST) {
    return found; // Allowed only if in list
  } else {
    return !found; // Allowed only if NOT in list
  }
}

std::vector<std::string> DomainPolicy::getAllDomains() const {
  return std::vector<std::string>(domains_.begin(), domains_.end());
}

void DomainPolicy::clear() { domains_.clear(); }

nlohmann::json DomainPolicy::toJson() const {
  nlohmann::json j;
  j["mode"] =
      (mode_ == DomainFilterMode::WHITELIST) ? "whitelist" : "blacklist";
  j["domains"] = domains_;
  return j;
}

DomainPolicy DomainPolicy::fromJson(const nlohmann::json &j) {
  DomainPolicy policy;

  if (j.contains("mode")) {
    std::string modeStr = j["mode"];
    if (modeStr == "whitelist") {
      policy.setMode(DomainFilterMode::WHITELIST);
    } else {
      policy.setMode(DomainFilterMode::BLACKLIST);
    }
  }

  if (j.contains("domains")) {
    for (const auto &domain : j["domains"]) {
      policy.addDomain(domain.get<std::string>());
    }
  }

  return policy;
}

} // namespace cms
