#include "cms/ui/DomainPolicyDialog.h"
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>


namespace cms {
namespace ui {

DomainPolicyDialog::DomainPolicyDialog(const DomainPolicy &currentPolicy,
                                       QWidget *parent)
    : QDialog(parent), policy_(currentPolicy) {
  setupUi();

  // Load policy into UI
  if (policy_.getMode() == DomainFilterMode::WHITELIST) {
    whitelistRadio_->setChecked(true);
  } else {
    blacklistRadio_->setChecked(true);
  }

  for (const auto &domain : policy_.getAllDomains()) {
    domainList_->addItem(QString::fromStdString(domain));
  }

  updateUiState();

  setWindowTitle("Domain Policy Management");
  resize(500, 400);
}

DomainPolicy DomainPolicyDialog::getPolicy() const { return policy_; }

void DomainPolicyDialog::setupUi() {
  auto mainLayout = new QVBoxLayout(this);

  // Mode Selection
  auto modeGroup = new QGroupBox("Filter Mode", this);
  auto modeLayout = new QVBoxLayout(modeGroup);

  whitelistRadio_ =
      new QRadioButton("Whitelist (Block all except listed)", this);
  blacklistRadio_ =
      new QRadioButton("Blacklist (Allow all except listed)", this);

  modeLayout->addWidget(whitelistRadio_);
  modeLayout->addWidget(blacklistRadio_);

  connect(whitelistRadio_, &QRadioButton::toggled, this,
          &DomainPolicyDialog::onModeChanged);
  connect(blacklistRadio_, &QRadioButton::toggled, this,
          &DomainPolicyDialog::onModeChanged);

  mainLayout->addWidget(modeGroup);

  // Domain List
  auto listGroup = new QGroupBox("Domains", this);
  auto listLayout = new QVBoxLayout(listGroup);

  domainList_ = new QListWidget(this);
  listLayout->addWidget(domainList_);

  // Input Area
  auto inputLayout = new QHBoxLayout();
  domainInput_ = new QLineEdit(this);
  domainInput_->setPlaceholderText("example.com");

  addButton_ = new QPushButton("Add", this);
  removeButton_ = new QPushButton("Remove", this);

  inputLayout->addWidget(domainInput_);
  inputLayout->addWidget(addButton_);
  inputLayout->addWidget(removeButton_);

  listLayout->addLayout(inputLayout);

  connect(addButton_, &QPushButton::clicked, this,
          &DomainPolicyDialog::onAddClicked);
  connect(removeButton_, &QPushButton::clicked, this,
          &DomainPolicyDialog::onRemoveClicked);
  connect(domainInput_, &QLineEdit::returnPressed, this,
          &DomainPolicyDialog::onAddClicked);

  mainLayout->addWidget(listGroup);

  // Dialog Buttons
  auto buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  mainLayout->addWidget(buttonBox);
}

void DomainPolicyDialog::onAddClicked() {
  QString domain = domainInput_->text().trimmed();
  if (domain.isEmpty()) {
    return;
  }

  // Basic validation (can be improved)
  if (!domain.contains(".")) {
    QMessageBox::warning(this, "Invalid Domain",
                         "Please enter a valid domain (e.g., example.com)");
    return;
  }

  // Check duplicate
  auto items = domainList_->findItems(domain, Qt::MatchExactly);
  if (!items.isEmpty()) {
    return; // Already exists
  }

  domainList_->addItem(domain);
  policy_.addDomain(domain.toStdString());
  domainInput_->clear();
}

void DomainPolicyDialog::onRemoveClicked() {
  auto items = domainList_->selectedItems();
  if (items.isEmpty()) {
    return;
  }

  for (auto item : items) {
    policy_.removeDomain(item->text().toStdString());
    delete domainList_->takeItem(domainList_->row(item));
  }
}

void DomainPolicyDialog::onModeChanged() {
  if (whitelistRadio_->isChecked()) {
    policy_.setMode(DomainFilterMode::WHITELIST);
  } else {
    policy_.setMode(DomainFilterMode::BLACKLIST);
  }
}

void DomainPolicyDialog::updateUiState() {
  // Can be used to enable/disable controls based on state
}

} // namespace ui
} // namespace cms
