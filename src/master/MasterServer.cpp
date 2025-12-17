#include "cms/MasterServer.h"
#include "cms/Logger.h"

namespace cms {
namespace master {

class MasterServerImpl : public MasterServer {
public:
    explicit MasterServerImpl(const ServerConfig& config) : config_(config) {}
    
    bool start() override {
        LOG_INFO("Master Server started on port " + std::to_string(config_.port));
        return true;
    }
    
    bool stop() override {
        LOG_INFO("Master Server stopped");
        return true;
    }
    
    bool isRunning() const override {
        return false;
    }

    std::vector<ClientInfo> getConnectedClients() const override {
        return {};
    }
    
    ClientInfo getClientInfo(const std::string& client_id) const override {
        return ClientInfo{};
    }
    
    bool lockClient(const std::string& client_id) override { return true; }
    bool unlockClient(const std::string& client_id) override { return true; }
    bool lockAll() override { return true; }
    bool unlockAll() override { return true; }
    
    bool broadcastMessage(const std::string& message) override { return true; }
    bool refreshClientThumbnail(const std::string& client_id) override { return true; }

private:
    ServerConfig config_;
};

std::unique_ptr<MasterServer> createMasterServer(const ServerConfig& config) {
    return std::make_unique<MasterServerImpl>(config);
}

} // namespace master
} // namespace cms
