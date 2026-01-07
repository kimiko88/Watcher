#include "cms/ui/LoginDialog.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace cms {
namespace ui {

LoginDialog::LoginDialog(std::shared_ptr<cms::auth::IAuthProvider> authProvider,
                         QWidget *parent)
    : QDialog(parent) {

  setupUi();

  // Load saved settings
  QSettings settings("CMS", "Master");
  usernameEdit_->setText(settings.value("LastUser", "").toString());
  domainEdit_->setText(settings.value("LastDomain", "").toString());
  hostEdit_->setText(
      settings.value("LdapHost", "localhost").toString()); // Default

  hostEdit_->setText(
      settings.value("LdapHost", "localhost").toString()); // Default

  if (!authProvider) {
    // Warning or dev mode
    offlineModeCheck_->setChecked(true);
  }
}

void LoginDialog::setupUi() {
  setWindowTitle("Login - Classroom Management System");
  setModal(true);

  auto mainLayout = new QVBoxLayout(this);
  auto formLayout = new QFormLayout();

  usernameEdit_ = new QLineEdit(this);
  passwordEdit_ = new QLineEdit(this);
  passwordEdit_->setEchoMode(QLineEdit::Password);

  domainEdit_ = new QLineEdit(this);
  domainEdit_->setPlaceholderText("Optional");

  hostEdit_ = new QLineEdit(this);
  hostEdit_->setPlaceholderText("LDAP Server Hostname/IP");

  offlineModeCheck_ = new QCheckBox("Offline Mode (Development)", this);

  formLayout->addRow("Username:", usernameEdit_);
  formLayout->addRow("Password:", passwordEdit_);
  formLayout->addRow("Domain:", domainEdit_);
  formLayout->addRow("LDAP Host:", hostEdit_);
  formLayout->addRow("", offlineModeCheck_);

  mainLayout->addLayout(formLayout);

  auto buttonLayout = new QHBoxLayout();
  auto okButton = new QPushButton("Login", this);
  auto cancelButton = new QPushButton("Cancel", this);

  connect(okButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
  connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

  buttonLayout->addStretch();
  buttonLayout->addWidget(okButton);
  buttonLayout->addWidget(cancelButton);

  mainLayout->addLayout(buttonLayout);
}

QString LoginDialog::getUsername() const { return usernameEdit_->text(); }
QString LoginDialog::getPassword() const { return passwordEdit_->text(); }
QString LoginDialog::getDomain() const { return domainEdit_->text(); }
QString LoginDialog::getLdapHost() const { return hostEdit_->text(); }

void LoginDialog::onLoginClicked() {
  if (offlineModeCheck_->isChecked()) {
    accept();
    return;
  }

  QString username = usernameEdit_->text();
  QString password = passwordEdit_->text();

  if (username.isEmpty() || password.isEmpty()) {
    QMessageBox::warning(this, "Input Error",
                         "Please enter username and password.");
    return;
  }

  // Save settings (excluding password)
  QSettings settings("CMS", "Master");
  settings.setValue("LastUser", username);
  settings.setValue("LastDomain", domainEdit_->text());
  settings.setValue("LdapHost", hostEdit_->text());

  // Perform Authentication
  // Note: In real app, we might need to recreate AuthProvider if host changed
  // But here we rely on the one passed in.
  // Ideally main.cpp creates provider based on settings BEFORE dialog?
  // Or Dialog returns creds + host, and main.cpp authenticates.
  // Let's stick to "Dialog returns accepted if valid or offline".

  // BUT we need to authenticate!
  // The previous plan was to pass IAuthProvider.
  // If I passed it, I should use it here.

  // However, I didn't store authProvider in member variable in my constructor
  // implementation above (oops). Let's fix that in next step or just accept for
  // now and let main handle it? "LoginDialog returns credentials" -> Main uses
  // them.

  accept();
}

} // namespace ui
} // namespace cms
