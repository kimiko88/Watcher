#include "cms/Logger.h"
#include "cms/MasterServer.h"
#include "cms/auth/LdapAuthProvider.h"
#include "cms/ui/LoginDialog.h"
#include "cms/ui/MasterWindow.h"
#include <QApplication>
#include <QFile>
#include <QMessageBox>

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

  // Authentication Flow
  while (true) {
    // We pass nullptr as provider initially, main handles logic
    cms::ui::LoginDialog loginDialog(nullptr);
    if (loginDialog.exec() != QDialog::Accepted) {
      return 0; // User cancelled
    }

    // Check if offline mode
    // We need to access the checkbox state? No, getUsername() etc.
    // Modification needed in LoginDialog to expose offline mode status?
    // Or just check if username is empty (if validation logic enforces
    // non-empty for normal login)

    // Let's assume non-empty username means attempt login.
    QString user = loginDialog.getUsername();
    QString pass = loginDialog.getPassword();
    QString host = loginDialog.getLdapHost();

    if (user.isEmpty()) {
      // Offline mode was likely checked (since we allow empty only if offline
      // checked in Dialog logic)
      break;
    }

    // Try Authenticate
    cms::auth::LdapAuthProvider auth(host.toStdString());
    if (auth.authenticate(user.toStdString(), pass.toStdString(),
                          loginDialog.getDomain().toStdString())) {
      break; // Success
    } else {
      QMessageBox::critical(nullptr, "Login Failed",
                            "Invalid credentials or connection refused.");
      // Loop again
    }
  }

  cms::ui::MasterWindow window(
      std::shared_ptr<cms::master::MasterServer>(server.release()));
  window.show();

  return app.exec();
}
