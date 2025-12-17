#include <QApplication>
#include "cms/ui/MasterWindow.h"
#include "cms/MasterServer.h"
#include "cms/Logger.h"

int main(int argc, char *argv[]) {
    // Init Logger
    cms::Logger::getInstance().init("master_log.txt", true);
    
    QApplication app(argc, argv);
    
    cms::master::ServerConfig config;
    auto server = cms::master::createMasterServer(config);
    server->start();
    
    cms::ui::MasterWindow window(std::shared_ptr<cms::master::MasterServer>(server.release()));
    window.show();
    
    return app.exec();
}
