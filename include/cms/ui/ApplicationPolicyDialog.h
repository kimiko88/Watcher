#ifndef CMS_UI_APPLICATION_POLICY_DIALOG_H
#define CMS_UI_APPLICATION_POLICY_DIALOG_H

#include "cms/ApplicationManager.h"
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <vector>

namespace cms {
namespace ui {

// Container for application policy data
struct ApplicationPolicy {
  AppFilterMode mode;
  std::vector<ApplicationRule> rules;
};

class ApplicationPolicyDialog : public QDialog {
  Q_OBJECT

public:
  explicit ApplicationPolicyDialog(const ApplicationPolicy &currentPolicy,
                                   QWidget *parent = nullptr);

  // Get the configured policy
  ApplicationPolicy getPolicy() const;

private slots:
  void onAddClicked();
  void onRemoveClicked();
  void onBrowseClicked();
  void onModeChanged(int index);
  void onImportClicked();
  void onExportClicked();

private:
  void setupUi();
  void loadRulesIntoTable();
  void updateButtonStates();

  // UI Components
  QComboBox *modeComboBox_;
  QTableWidget *rulesTable_;
  QPushButton *addButton_;
  QPushButton *removeButton_;
  QPushButton *browseButton_;
  QPushButton *importButton_;
  QPushButton *exportButton_;
  QLineEdit *appPathEdit_;
  QLineEdit *appNameEdit_;
  QLineEdit *regexEdit_;
  QComboBox *actionComboBox_;

  // Data
  ApplicationPolicy policy_;
};

} // namespace ui
} // namespace cms

#endif // CMS_UI_APPLICATION_POLICY_DIALOG_H
