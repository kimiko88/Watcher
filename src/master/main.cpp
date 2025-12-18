#include "cms/Logger.h"
#include "cms/MasterServer.h"
#include "cms/ui/MasterWindow.h"
#include <QApplication>
#include <QFile>


int main(int argc, char *argv[]) {
  // Init Logger
  cms::Logger::getInstance().init("master_log.txt", true);

  QApplication app(argc, argv);

  // Load modern stylesheet
  QFile styleFile("../src/master/resources/style.qss");
  if (styleFile.open(QFile::ReadOnly)) {
    QString styleSheet = QString::fromLatin1(styleFile.readAll());
    app.setStyleSheet(styleSheet);
    styleFile.close();
  }

  cms::master::ServerConfig config;
  auto server = cms::master::createMasterServer(config);
  server->start();

  cms::ui::MasterWindow window(
      std::shared_ptr<cms::master::MasterServer>(server.release()));
  window.show();

  return app.exec();
}
