#include "cms/ui/ApplicationPolicyDialog.h"
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace cms {
namespace ui {

ApplicationPolicyDialog::ApplicationPolicyDialog(
    const ApplicationPolicy &currentPolicy, QWidget *parent)
    : QDialog(parent), policy_(currentPolicy) {
  setupUi();
  loadRulesIntoTable();
  updateButtonStates();

  setWindowTitle("Application Policy Management");
  resize(800, 500);
}

ApplicationPolicy ApplicationPolicyDialog::getPolicy() const { return policy_; }

void ApplicationPolicyDialog::setupUi() {
  auto mainLayout = new QVBoxLayout(this);

  // Mode Selection Group
  auto modeGroup = new QGroupBox("Filter Mode", this);
  auto modeLayout = new QHBoxLayout(modeGroup);

  modeLayout->addWidget(new QLabel("Mode:", this));
  modeComboBox_ = new QComboBox(this);
  modeComboBox_->addItem("Disabled",
                         static_cast<int>(AppFilterMode::MODE_DISABLED));
  modeComboBox_->addItem("Blacklist (Block listed apps)",
                         static_cast<int>(AppFilterMode::MODE_BLACKLIST));
  modeComboBox_->addItem("Whitelist (Allow only listed apps)",
                         static_cast<int>(AppFilterMode::MODE_WHITELIST));

  // Set current mode
  int currentIndex = 0;
  if (policy_.mode == AppFilterMode::MODE_BLACKLIST)
    currentIndex = 1;
  else if (policy_.mode == AppFilterMode::MODE_WHITELIST)
    currentIndex = 2;
  modeComboBox_->setCurrentIndex(currentIndex);

  connect(modeComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ApplicationPolicyDialog::onModeChanged);

  modeLayout->addWidget(modeComboBox_);
  modeLayout->addStretch();
  mainLayout->addWidget(modeGroup);

  // Rules Table Group
  auto rulesGroup = new QGroupBox("Application Rules", this);
  auto rulesLayout = new QVBoxLayout(rulesGroup);

  rulesTable_ = new QTableWidget(this);
  rulesTable_->setColumnCount(5);
  rulesTable_->setHorizontalHeaderLabels(
      {"App Name", "Path", "Process Pattern", "Action", "Enabled"});
  rulesTable_->horizontalHeader()->setStretchLastSection(false);
  rulesTable_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  rulesTable_->horizontalHeader()->setSectionResizeMode(1,
                                                        QHeaderView::Stretch);
  rulesTable_->horizontalHeader()->setSectionResizeMode(2,
                                                        QHeaderView::Stretch);
  rulesTable_->horizontalHeader()->setSectionResizeMode(
      3, QHeaderView::ResizeToContents);
  rulesTable_->horizontalHeader()->setSectionResizeMode(
      4, QHeaderView::ResizeToContents);
  rulesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
  rulesTable_->setSelectionMode(QAbstractItemView::SingleSelection);
  rulesLayout->addWidget(rulesTable_);

  // Input Area
  auto inputLayout = new QGridLayout();
  int row = 0;

  inputLayout->addWidget(new QLabel("App Name:", this), row, 0);
  appNameEdit_ = new QLineEdit(this);
  appNameEdit_->setPlaceholderText("e.g., Google Chrome");
  inputLayout->addWidget(appNameEdit_, row, 1);
  row++;

  inputLayout->addWidget(new QLabel("App Path:", this), row, 0);
  appPathEdit_ = new QLineEdit(this);
  appPathEdit_->setPlaceholderText("e.g., C:\\Program Files\\App\\app.exe");
  inputLayout->addWidget(appPathEdit_, row, 1);

  browseButton_ = new QPushButton("Browse...", this);
  connect(browseButton_, &QPushButton::clicked, this,
          &ApplicationPolicyDialog::onBrowseClicked);
  inputLayout->addWidget(browseButton_, row, 2);
  row++;

  inputLayout->addWidget(new QLabel("Process Pattern (regex):", this), row, 0);
  regexEdit_ = new QLineEdit(this);
  regexEdit_->setPlaceholderText("e.g., .*chrome.* (optional)");
  inputLayout->addWidget(regexEdit_, row, 1);
  row++;

  inputLayout->addWidget(new QLabel("Action:", this), row, 0);
  actionComboBox_ = new QComboBox(this);
  actionComboBox_->addItem("Block", static_cast<int>(RuleAction::BLOCK));
  actionComboBox_->addItem("Allow", static_cast<int>(RuleAction::ALLOW));
  inputLayout->addWidget(actionComboBox_, row, 1);
  row++;

  rulesLayout->addLayout(inputLayout);

  // Button Area
  auto buttonLayout = new QHBoxLayout();
  addButton_ = new QPushButton("Add Rule", this);
  removeButton_ = new QPushButton("Remove Selected", this);
  importButton_ = new QPushButton("Import CSV...", this);
  exportButton_ = new QPushButton("Export CSV...", this);

  connect(addButton_, &QPushButton::clicked, this,
          &ApplicationPolicyDialog::onAddClicked);
  connect(removeButton_, &QPushButton::clicked, this,
          &ApplicationPolicyDialog::onRemoveClicked);
  connect(importButton_, &QPushButton::clicked, this,
          &ApplicationPolicyDialog::onImportClicked);
  connect(exportButton_, &QPushButton::clicked, this,
          &ApplicationPolicyDialog::onExportClicked);

  buttonLayout->addWidget(addButton_);
  buttonLayout->addWidget(removeButton_);
  buttonLayout->addStretch();
  buttonLayout->addWidget(importButton_);
  buttonLayout->addWidget(exportButton_);

  rulesLayout->addLayout(buttonLayout);
  mainLayout->addWidget(rulesGroup);

  // Dialog Buttons
  auto dialogButtonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  mainLayout->addWidget(dialogButtonBox);
}

void ApplicationPolicyDialog::loadRulesIntoTable() {
  rulesTable_->setRowCount(0);

  for (const auto &rule : policy_.rules) {
    int row = rulesTable_->rowCount();
    rulesTable_->insertRow(row);

    rulesTable_->setItem(
        row, 0, new QTableWidgetItem(QString::fromStdString(rule.app_name)));
    rulesTable_->setItem(
        row, 1, new QTableWidgetItem(QString::fromStdString(rule.app_path)));
    rulesTable_->setItem(
        row, 2,
        new QTableWidgetItem(QString::fromStdString(rule.process_pattern)));
    rulesTable_->setItem(row, 3,
                         new QTableWidgetItem(rule.action == RuleAction::BLOCK
                                                  ? "Block"
                                                  : "Allow"));
    rulesTable_->setItem(row, 4,
                         new QTableWidgetItem(rule.enabled ? "Yes" : "No"));
  }
}

void ApplicationPolicyDialog::onAddClicked() {
  QString appName = appNameEdit_->text().trimmed();
  QString appPath = appPathEdit_->text().trimmed();
  QString regex = regexEdit_->text().trimmed();

  if (appName.isEmpty() || (appPath.isEmpty() && regex.isEmpty())) {
    QMessageBox::warning(
        this, "Invalid Input",
        "Please provide at least App Name and either Path or Pattern.");
    return;
  }

  ApplicationRule rule;
  rule.rule_id = std::to_string(
      std::hash<std::string>{}(appPath.toStdString() + appName.toStdString()));
  rule.app_name = appName.toStdString();
  rule.app_path = appPath.toStdString();
  rule.process_pattern = regex.toStdString();
  rule.action = static_cast<RuleAction>(actionComboBox_->currentData().toInt());
  rule.enabled = true;
  rule.created_at = 0;

  policy_.rules.push_back(rule);
  loadRulesIntoTable();

  // Clear inputs
  appNameEdit_->clear();
  appPathEdit_->clear();
  regexEdit_->clear();
}

void ApplicationPolicyDialog::onRemoveClicked() {
  auto selectedItems = rulesTable_->selectedItems();
  if (selectedItems.isEmpty()) {
    return;
  }

  int row = rulesTable_->row(selectedItems.first());
  if (row >= 0 && row < static_cast<int>(policy_.rules.size())) {
    policy_.rules.erase(policy_.rules.begin() + row);
    loadRulesIntoTable();
  }
}

void ApplicationPolicyDialog::onBrowseClicked() {
  QString filename = QFileDialog::getOpenFileName(
      this, "Select Application Executable", QString(),
      "Executables (*.exe);;All Files (*)");

  if (!filename.isEmpty()) {
    appPathEdit_->setText(filename);

    // Auto-fill app name from filename
    if (appNameEdit_->text().isEmpty()) {
      QFileInfo fileInfo(filename);
      appNameEdit_->setText(fileInfo.baseName());
    }
  }
}

void ApplicationPolicyDialog::onModeChanged(int index) {
  policy_.mode =
      static_cast<AppFilterMode>(modeComboBox_->itemData(index).toInt());
}

void ApplicationPolicyDialog::onImportClicked() {
  QString filename = QFileDialog::getOpenFileName(
      this, "Import Application Rules", QString(), "CSV Files (*.csv)");

  if (!filename.isEmpty()) {
    // TODO: Implement CSV import logic
    QMessageBox::information(this, "Import",
                             "CSV import feature not yet implemented");
  }
}

void ApplicationPolicyDialog::onExportClicked() {
  QString filename = QFileDialog::getSaveFileName(
      this, "Export Application Rules", "app_rules.csv", "CSV Files (*.csv)");

  if (!filename.isEmpty()) {
    // TODO: Implement CSV export logic
    QMessageBox::information(this, "Export",
                             "CSV export feature not yet implemented");
  }
}

void ApplicationPolicyDialog::updateButtonStates() {
  // Enable/disable buttons based on state
  removeButton_->setEnabled(rulesTable_->rowCount() > 0);
  exportButton_->setEnabled(rulesTable_->rowCount() > 0);
}

} // namespace ui
} // namespace cms
