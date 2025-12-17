#include "cms/ui/MasterWindow.h"
#include "cms/ui/ClientThumbnailWidget.h"
#include <QToolbar>
#include <QStatusBar>
#include <QLabel>
#include <QAction>

namespace cms {
namespace ui {

MasterWindow::MasterWindow(std::shared_ptr<master::MasterServer> server, QWidget* parent)
    : QMainWindow(parent)
    , server_(server)
{
    model_ = new ClientListModel(this);
    
    setupUi();
    setupToolbar();
    
    resize(1024, 768);
    setWindowTitle("Classroom Management System - Master");
}

MasterWindow::~MasterWindow() {
}

void MasterWindow::setupUi() {
    // Basic placeholder central widget
    central_widget_ = new QLabel("Client Grid View Placeholder", this);
    static_cast<QLabel*>(central_widget_)->setAlignment(Qt::AlignCenter);
    setCentralWidget(central_widget_);
    
    statusBar()->showMessage("Ready");
}

void MasterWindow::setupToolbar() {
    auto toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    
    auto lockAction = toolbar->addAction("Lock All");
    connect(lockAction, &QAction::triggered, this, &MasterWindow::onLockAll);
    
    auto unlockAction = toolbar->addAction("Unlock All");
    connect(unlockAction, &QAction::triggered, this, &MasterWindow::onUnlockAll);
    
    toolbar->addSeparator();
    
    auto refreshAction = toolbar->addAction("Refresh");
    connect(refreshAction, &QAction::triggered, this, &MasterWindow::onRefresh);
}

void MasterWindow::onRefresh() {
    statusBar()->showMessage("Refreshing...");
}

void MasterWindow::onLockAll() {
    server_->lockAll();
    statusBar()->showMessage("Locked all clients");
}

void MasterWindow::onUnlockAll() {
    server_->unlockAll();
    statusBar()->showMessage("Unlocked all clients");
}

} // namespace ui
} // namespace cms
