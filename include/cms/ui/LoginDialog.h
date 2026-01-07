#ifndef CMS_UI_LOGIN_DIALOG_H
#define CMS_UI_LOGIN_DIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>

#include "cms/auth/IAuthProvider.h"
#include <memory>

namespace cms {
namespace ui {

class LoginDialog : public QDialog {
  Q_OBJECT

public:
  explicit LoginDialog(
      std::shared_ptr<cms::auth::IAuthProvider> authProvider = nullptr,
      QWidget *parent = nullptr);

  QString getUsername() const;
  QString getPassword() const;
  QString getDomain() const;
  QString getLdapHost() const;

private slots:
  void onLoginClicked();

private:
  QLineEdit *usernameEdit_;
  QLineEdit *passwordEdit_;
  QLineEdit *domainEdit_;
  QLineEdit *hostEdit_;
  QCheckBox *offlineModeCheck_; // Allow skipping auth for dev/demo

  void setupUi();
};

} // namespace ui
} // namespace cms

#endif // CMS_UI_LOGIN_DIALOG_H
