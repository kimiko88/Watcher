#ifndef CMS_UI_DOMAIN_POLICY_DIALOG_H
#define CMS_UI_DOMAIN_POLICY_DIALOG_H

#include "cms/DomainPolicy.h"
#include <QDialog>


class QListWidget;
class QRadioButton;
class QLineEdit;
class QPushButton;

namespace cms {
namespace ui {

class DomainPolicyDialog : public QDialog {
  Q_OBJECT

public:
  explicit DomainPolicyDialog(const DomainPolicy &currentPolicy,
                              QWidget *parent = nullptr);

  DomainPolicy getPolicy() const;

private slots:
  void onAddClicked();
  void onRemoveClicked();
  void onModeChanged();

private:
  void setupUi();
  void updateUiState();

  DomainPolicy policy_;

  // UI Elements
  QRadioButton *whitelistRadio_;
  QRadioButton *blacklistRadio_;
  QListWidget *domainList_;
  QLineEdit *domainInput_;
  QPushButton *addButton_;
  QPushButton *removeButton_;
};

} // namespace ui
} // namespace cms

#endif // CMS_UI_DOMAIN_POLICY_DIALOG_H
