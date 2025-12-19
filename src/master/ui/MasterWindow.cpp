#include "cms/ui/MasterWindow.h"
#include "cms/Protocol.h"
#include "cms/ui/ApplicationPolicyDialog.h"
#include "cms/ui/ClientThumbnailWidget.h"
#include "cms/ui/DomainPolicyDialog.h"
#include "cms/ui/RemoteViewWindow.h"
#include "cms/ui/ScreenshotViewerDialog.h"
#include <QAction>
#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollArea>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <iostream>

namespace cms {
namespace ui {

MasterWindow::MasterWindow(std::shared_ptr<master::MasterServer> server,
                           QWidget *parent)
    : QMainWindow(parent), server_(server) {
  model_ = new ClientListModel(this);

  setupUi();
  setupMenuBar();
  setupToolbar();

  server_->addObserver(model_);
  server_->addObserver(this);

  // Connect model changes to grid updates
  connect(model_, &QAbstractTableModel::rowsInserted, this,
          &MasterWindow::refreshGrid);
  connect(model_, &QAbstractTableModel::modelReset, this,
          &MasterWindow::refreshGrid);

  resize(1024, 768);
  setWindowTitle("Classroom Management System - Master");

  // DEBUG: Simulate a client connection after 2 seconds
  QTimer::singleShot(2000, [this]() {
    // This is a hack because we know the implementation details.
    // In real code, we wouldn't cast. This is just to prove the UI works.
    // Dynamic_cast would require RTTI enabled everywhere.
    // Let's assume we can somehow trigger simulation or wait for real
    // connection.
  });
}

MasterWindow::~MasterWindow() {
  if (server_) {
    server_->removeObserver(model_);
    server_->removeObserver(this);
  }
}

void MasterWindow::setupUi() {
  central_widget_ = new QWidget(this);
  auto mainLayout = new QVBoxLayout(central_widget_);

  scroll_area_ = new QScrollArea(this);
  scroll_area_->setWidgetResizable(true);

  grid_container_ = new QWidget(scroll_area_);
  // Fallback if no FlowLayout: Use QGridLayout with fixed columns
  // actually let's use QGridLayout for simplicity MVP
  auto grid = new QGridLayout(grid_container_);
  grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);

  grid_container_->setLayout(grid);
  scroll_area_->setWidget(grid_container_);

  mainLayout->addWidget(scroll_area_);
  setCentralWidget(central_widget_);

  statusBar()->showMessage("Ready");
}

void MasterWindow::refreshGrid() {
  // Clear existing items provided they are widgets
  QLayoutItem *child;
  auto layout = grid_container_->layout();
  while ((child = layout->takeAt(0)) != nullptr) {
    if (child->widget()) {
      delete child->widget();
    }
    delete child;
  }

  // Rebuild grid
  auto *grid = static_cast<QGridLayout *>(layout);
  int columns = 3; // Fixed columns for now

  for (int i = 0; i < model_->rowCount(); ++i) {
    auto client = model_->getClient(i);
    auto widget = new ClientThumbnailWidget(grid_container_);
    widget->setClientInfo(client);
    connect(widget, &ClientThumbnailWidget::clicked, this,
            &MasterWindow::onClientThumbnailClicked);

    connect(widget, &ClientThumbnailWidget::lockRequested,
            [this](const std::string &id) {
              server_->lockClient(id);
              statusBar()->showMessage("Locked client: " +
                                       QString::fromStdString(id));
            });

    connect(widget, &ClientThumbnailWidget::unlockRequested,
            [this](const std::string &id) {
              server_->unlockClient(id);
              statusBar()->showMessage("Unlocked client: " +
                                       QString::fromStdString(id));
            });

    connect(widget, &ClientThumbnailWidget::screenshotRequested,
            [this](const std::string &id) {
              server_->refreshClientThumbnail(id);
              statusBar()->showMessage("Requested screenshot from: " +
                                       QString::fromStdString(id));
            });

    // Disconnect is not yet in MasterServer interface, so we'll just log for
    // now or skip
    connect(widget, &ClientThumbnailWidget::disconnectRequested,
            [this](const std::string &id) {
              // server_->disconnectClient(id); // TODO: Implement in
              // MasterServer
              statusBar()->showMessage("Disconnect requested for: " +
                                       QString::fromStdString(id));
            });

    int row = i / columns;
    int col = i % columns;
    grid->addWidget(widget, row, col);
  }

  statusBar()->showMessage(QString("Clients: %1").arg(model_->rowCount()));
}

void MasterWindow::setupMenuBar() {
  auto fileMenu = menuBar()->addMenu("&File");
  auto exitAction = fileMenu->addAction("E&xit");
  exitAction->setShortcut(QKeySequence::Quit);
  connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

  auto viewMenu = menuBar()->addMenu("&View");
  auto refreshAction = viewMenu->addAction("&Refresh");
  refreshAction->setShortcut(QKeySequence::Refresh);
  connect(refreshAction, &QAction::triggered, this, &MasterWindow::onRefresh);

  auto toolsMenu = menuBar()->addMenu("&Tools");
  auto screenshotAction = toolsMenu->addAction("Take &Screenshot");
  screenshotAction->setShortcut(QKeySequence("Ctrl+S"));
  connect(screenshotAction, &QAction::triggered, this,
          &MasterWindow::onTakeScreenshot);

  toolsMenu->addSeparator();
  auto domainPolicyAction = toolsMenu->addAction("&Domain Policy...");
  connect(domainPolicyAction, &QAction::triggered, this,
          &MasterWindow::onDomainPolicyClicked);

  auto appPolicyAction = toolsMenu->addAction("&Application Policy...");
  connect(appPolicyAction, &QAction::triggered, this,
          &MasterWindow::onApplicationPolicyClicked);

  toolsMenu->addSeparator();
  auto lockAllAction = toolsMenu->addAction("&Lock All Clients");
  lockAllAction->setShortcut(QKeySequence("Ctrl+L"));
  connect(lockAllAction, &QAction::triggered, this, &MasterWindow::onLockAll);

  auto unlockAllAction = toolsMenu->addAction("&Unlock All Clients");
  unlockAllAction->setShortcut(QKeySequence("Ctrl+U"));
  connect(unlockAllAction, &QAction::triggered, this,
          &MasterWindow::onUnlockAll);

  auto helpMenu = menuBar()->addMenu("&Help");
  auto aboutAction = helpMenu->addAction("&About");
  connect(aboutAction, &QAction::triggered, this,
          &MasterWindow::onAboutClicked);
}

void MasterWindow::setupToolbar() {
  auto toolbar = addToolBar("Main Toolbar");
  toolbar->setMovable(false);

  auto lockAction = toolbar->addAction("Lock All");
  connect(lockAction, &QAction::triggered, this, &MasterWindow::onLockAll);

  auto unlockAction = toolbar->addAction("Unlock All");
  connect(unlockAction, &QAction::triggered, this, &MasterWindow::onUnlockAll);

  toolbar->addSeparator();

  auto screenshotAction = toolbar->addAction("📸 Screenshot");
  screenshotAction->setToolTip("Take screenshot from selected client");
  connect(screenshotAction, &QAction::triggered, this,
          &MasterWindow::onTakeScreenshot);

  toolbar->addSeparator();

  auto domainPolicyAction = toolbar->addAction("🌐 Domains");
  domainPolicyAction->setToolTip("Configure domain policy");
  connect(domainPolicyAction, &QAction::triggered, this,
          &MasterWindow::onDomainPolicyClicked);

  auto appPolicyAction = toolbar->addAction("🎮 Apps");
  appPolicyAction->setToolTip("Configure application policy");
  connect(appPolicyAction, &QAction::triggered, this,
          &MasterWindow::onApplicationPolicyClicked);

  auto refreshAction = toolbar->addAction("Refresh");
  connect(refreshAction, &QAction::triggered, this, &MasterWindow::onRefresh);

  auto debugAddClient = toolbar->addAction("[DEBUG] Add Client");
  connect(debugAddClient, &QAction::triggered, [this]() {
    static int count = 1;
    master::ClientInfo info;
    info.id = "debug_" + std::to_string(count);
    info.hostname = "Student-" + std::to_string(count);
    info.ip_address = "192.168.1.10" + std::to_string(count);
    info.state = master::ClientState::CONNECTED;
    model_->onClientConnected(info); // Directly call observer method for debug
    count++;
  });
}

void MasterWindow::onRefresh() {
  statusBar()->showMessage("Refreshing...");
  // Trigger server discovery or refresh
}

void MasterWindow::onLockAll() {
  server_->lockAll();
  statusBar()->showMessage("Locked all clients");
}

void MasterWindow::onUnlockAll() {
  server_->unlockAll();
  statusBar()->showMessage("Unlocked all clients");
}

void MasterWindow::onTakeScreenshot() {
  if (model_->rowCount() == 0) {
    QMessageBox::information(
        this, "No Clients",
        "No clients connected. Please connect clients first.");
    return;
  }

  if (selected_client_id_.empty()) {
    QMessageBox::warning(this, "No Selection", "Please select a client first.");
    return;
  }

  server_->refreshClientThumbnail(selected_client_id_);
  statusBar()->showMessage("Requested screenshot from " +
                           QString::fromStdString(selected_client_id_));
}

void MasterWindow::onDomainPolicyClicked() {
  // Use current policy
  cms::DomainPolicy policy = current_policy_;

  DomainPolicyDialog dialog(policy, this);
  if (dialog.exec() == QDialog::Accepted) {
    // Save policy to server and broadcast
    current_policy_ = dialog.getPolicy();
    cms::DomainPolicy newPolicy = current_policy_;

    // Broadcast to all clients
    auto clients = server_->getConnectedClients();
    int count = 0;
    for (const auto &client : clients) {
      if (server_->sendDomainPolicy(client.id, newPolicy)) {
        count++;
      }
    }

    statusBar()->showMessage(
        QString("Domain policy sent to %1 clients").arg(count));
  }
}

void MasterWindow::onApplicationPolicyClicked() {
  // Initialize with empty policy if needed
  cms::ui::ApplicationPolicy policy;
  policy.mode = AppFilterMode::MODE_DISABLED;

  ApplicationPolicyDialog dialog(policy, this);
  if (dialog.exec() == QDialog::Accepted) {
    cms::ui::ApplicationPolicy newPolicy = dialog.getPolicy();

    // Create protocol message payload
    nlohmann::json payload;

    // Set mode
    std::string mode_str = "disabled";
    if (newPolicy.mode == AppFilterMode::MODE_BLACKLIST) {
      mode_str = "blacklist";
    } else if (newPolicy.mode == AppFilterMode::MODE_WHITELIST) {
      mode_str = "whitelist";
    }
    payload["mode"] = mode_str;

    // Add rules array
    nlohmann::json rules_array = nlohmann::json::array();
    for (const auto &rule : newPolicy.rules) {
      nlohmann::json rule_json;
      rule_json["rule_id"] = rule.rule_id;
      rule_json["app_path"] = rule.app_path;
      rule_json["app_name"] = rule.app_name;
      rule_json["process_pattern"] = rule.process_pattern;
      rule_json["action"] =
          (rule.action == RuleAction::BLOCK) ? "block" : "allow";
      rule_json["enabled"] = rule.enabled;
      rule_json["created_at"] = rule.created_at;
      rules_array.push_back(rule_json);
    }
    payload["rules"] = rules_array;

    // Send to all connected clients using Protocol messages
    auto clients = server_->getConnectedClients();
    int count = 0;
    protocol::MessageSerializer serializer;

    for (const auto &client : clients) {
      // Create APP_POLICY_SYNC message
      auto msg = protocol::Message::Create(
          protocol::CommandType::APP_POLICY_SYNC, "master", client.id, payload);

      // Serialize to JSON string
      std::string json = serializer.Serialize(msg);

      // Send to client (using sendToClient if available, or queue for sending)
      // For now, we'll log since sendToClient may not exist
      // In production, this would use server_->broadcastMessage or similar
      count++;

      // TODO: Replace with actual send when MasterServer API is ready
      // server_->sendMessageToClient(client.id, json);
    }

    statusBar()->showMessage(QString("Application policy configured for %1 "
                                     "client(s) (send pending implementation)")
                                 .arg(count));
  }
}

void MasterWindow::onAboutClicked() {
  QMessageBox::about(this, "About", "Classroom Management System\nVersion 1.0");
}

void MasterWindow::onScreenshotReceived(const std::string &client_id,
                                        const std::vector<uint8_t> &imageData) {
  QImage image;
  if (image.loadFromData(imageData.data(),
                         static_cast<int>(imageData.size()))) {
    auto client = server_->getClientInfo(client_id);

    // Check if we already have a dialog for this client?
    // For now, just open a new one.

    ScreenshotViewerDialog *dialog = new ScreenshotViewerDialog(
        image, QString::fromStdString(client.id),
        QString::fromStdString(client.hostname), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    statusBar()->showMessage("Screenshot received from " +
                             QString::fromStdString(client.hostname));
  } else {
    statusBar()->showMessage("Failed to load screenshot data");
  }
}

void MasterWindow::onClientThumbnailClicked(const std::string &client_id) {
  selected_client_id_ = client_id;
  statusBar()->showMessage("Selected client: " +
                           QString::fromStdString(client_id));

  // Optional: Open remote view on double click or separate action
  // For now, we just select.
  /*
  if (remote_views_.find(client_id) != remote_views_.end()) {
    remote_views_[client_id]->raise();
    remote_views_[client_id]->activateWindow();
    return;
  }

  auto *window = new RemoteViewWindow(client_id, server_, this);
  window->setAttribute(Qt::WA_DeleteOnClose);

  connect(window, &QWidget::destroyed,
          [this, client_id]() { remote_views_.erase(client_id); });

  remote_views_[client_id] = window;
  window->show();
  */
}

} // namespace ui
} // namespace cms
