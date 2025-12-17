#ifndef CMS_UI_MASTER_WINDOW_H
#define CMS_UI_MASTER_WINDOW_H

#include <QMainWindow>
#include "cms/MasterServer.h"
#include "cms/ui/ClientListModel.h"

class QListView;
class QGridView; // Placeholder if we make a custom grid view

namespace cms {
namespace ui {

class MasterWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MasterWindow(std::shared_ptr<master::MasterServer> server, QWidget* parent = nullptr);
    ~MasterWindow();

private slots:
    void onRefresh();
    void onLockAll();
    void onUnlockAll();

private:
    std::shared_ptr<master::MasterServer> server_;
    ClientListModel* model_;
    QWidget* central_widget_;
    
    void setupUi();
    void setupToolbar();
};

} // namespace ui
} // namespace cms

#endif // CMS_UI_MASTER_WINDOW_H
