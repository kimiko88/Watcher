#ifndef CMS_CONFIG_H
#define CMS_CONFIG_H

#include "Common.h"
#include <map>
#include <optional>

namespace cms {

// Configuration management system
class Config {
public:
    static Config& Instance() {
        static Config instance;
        return instance;
    }

    // Set a configuration value
    void Set(const String& key, const String& value) {
        config_[key] = value;
    }

    // Get a configuration value
    std::optional<String> Get(const String& key) const {
        auto it = config_.find(key);
        if (it != config_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // Get a configuration value with default
    String GetOr(const String& key, const String& defaultValue) const {
        auto value = Get(key);
        return value.value_or(defaultValue);
    }

    // Check if a key exists
    bool Has(const String& key) const {
        return config_.find(key) != config_.end();
    }

    // Remove a configuration value
    void Remove(const String& key) {
        config_.erase(key);
    }

    // Clear all configuration
    void Clear() {
        config_.clear();
    }

    // Get number of configuration entries
    size_t Size() const {
        return config_.size();
    }

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::map<String, String> config_;
};

} // namespace cms

#endif // CMS_CONFIG_H
