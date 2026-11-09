#include "gui/PatientDialog.h"

#include "controller/ClinicManager.h"
#include "gui/GuiHelpers.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/Exceptions.h"
#include "utils/Validator.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace scapms::gui {

using model::PatientCategory;

PatientDialog::PatientDialog(controller::ClinicManager& manager, QWidget* parent)
    : QDialog(parent), manager_(manager) {
    setWindowTitle(tr("Register New Patient"));
    buildForm();
    idEdit_->setText(qs(manager_.suggestedPatientId()));
    rebuildTypeFields(PatientCategory::General);
    nameEdit_->setFocus();
}

PatientDialog::PatientDialog(controller::ClinicManager& manager, const model::PatientRecord& existing,
                             QWidget* parent)
    : QDialog(parent), manager_(manager), editing_(true) {
    setWindowTitle(tr("Edit Patient %1").arg(qs(existing.id())));
    buildForm();
    fillFrom(existing);
}

void PatientDialog::buildForm() {
    setObjectName(QStringLiteral("patientDialog"));
    setMinimumWidth(520);

    form_ = new QFormLayout;
    form_->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form_->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form_->setVerticalSpacing(10);

    typeBox_ = new QComboBox;
    typeBox_->addItem(tr("General"), static_cast<int>(PatientCategory::General));
    typeBox_->addItem(tr("Emergency"), static_cast<int>(PatientCategory::Emergency));
    typeBox_->addItem(tr("Follow-Up"), static_cast<int>(PatientCategory::FollowUp));
    typeBox_->setToolTip(tr("The patient type decides which extra fields are shown and the scheduling rules."));
    form_->addRow(tr("Patient type:"), typeBox_);

    // ID field with a live availability hint next to it.
    idEdit_ = new QLineEdit;
    idEdit_->setMaxLength(8);
    idEdit_->setPlaceholderText(QStringLiteral("PAT-0001"));
    idEdit_->setToolTip(tr("Unique ID in the format PAT-0001. A free ID is suggested automatically."));
    idHint_ = new QLabel;
    auto* idRow = new QHBoxLayout;
    idRow->addWidget(idEdit_, 1);
    idRow->addWidget(idHint_);
    form_->addRow(tr("Patient ID:"), idRow);

    nameEdit_ = new QLineEdit;
    nameEdit_->setMaxLength(60);
    nameEdit_->setPlaceholderText(tr("e.g. Amina Bello"));
    nameEdit_->setToolTip(tr("Full name: letters, spaces, apostrophes, hyphens and full stops."));
    form_->addRow(tr("Full name:"), nameEdit_);

    ageEdit_ = new QLineEdit;
    ageEdit_->setMaxLength(4);
    ageEdit_->setPlaceholderText(tr("0 - 120"));
    ageEdit_->setToolTip(tr("Age in whole years, between 0 and 120."));
    ageHint_ = new QLabel;
    auto* ageRow = new QHBoxLayout;
    ageRow->addWidget(ageEdit_, 1);
    ageRow->addWidget(ageHint_);
    form_->addRow(tr("Age:"), ageRow);

    contactEdit_ = new QLineEdit;
    contactEdit_->setMaxLength(60);
    contactEdit_->setPlaceholderText(tr("08031234567 or name@miva.edu.ng"));
    contactEdit_->setToolTip(tr("Phone number or e-mail address."));
    form_->addRow(tr("Contact:"), contactEdit_);

    baseRowCount_ = form_->rowCount();   // rows after this index are type-specific

    typeNote_ = new QLabel;
    typeNote_->setObjectName(QStringLiteral("hintLabel"));
    typeNote_->setWordWrap(true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setText(editing_ ? tr("Save Changes") : tr("Register Patient"));
    buttons->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("primaryButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, &PatientDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &PatientDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form_);
    layout->addWidget(typeNote_);
    layout->addStretch();
    layout->addWidget(buttons);

    connect(typeBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, &PatientDialog::onTypeChanged);
    connect(idEdit_, &QLineEdit::textChanged, this, &PatientDialog::onIdEdited);
    connect(ageEdit_, &QLineEdit::textChanged, this, &PatientDialog::onAgeEdited);

    if (editing_) {
        idEdit_->setReadOnly(true);
        typeBox_->setEnabled(false);
        typeBox_->setToolTip(tr("The patient type cannot be changed after registration."));
    }
}

model::PatientCategory PatientDialog::selectedCategory() const {
    return static_cast<PatientCategory>(typeBox_->currentData().toInt());
}

void PatientDialog::onTypeChanged(int) {
    rebuildTypeFields(selectedCategory());
}

void PatientDialog::rebuildTypeFields(PatientCategory category) {
    // Remove the rows of the previous type (removeRow also deletes their widgets) ...
    while (form_->rowCount() > baseRowCount_) form_->removeRow(baseRowCount_);
    membershipBox_ = nullptr;
    severityBox_ = nullptr;
    complaintEdit_ = nullptr;
    diagnosisEdit_ = nullptr;
    lastVisitEdit_ = nullptr;

    // ... and add the rows the selected type needs.
    switch (category) {
        case PatientCategory::General:
            membershipBox_ = new QComboBox;
            for (const auto& membership : model::GeneralPatient::memberships()) membershipBox_->addItem(qs(membership));
            membershipBox_->setToolTip(tr("Relationship with the university."));
            form_->addRow(tr("Membership:"), membershipBox_);
            typeNote_->setText(tr("General patients are seen Monday to Friday, 08:00 - 17:00 (priority: Normal)."));
            break;
        case PatientCategory::Emergency:
            severityBox_ = new QSpinBox;
            severityBox_->setRange(model::EmergencyPatient::kMinSeverity, model::EmergencyPatient::kMaxSeverity);
            severityBox_->setValue(3);
            severityBox_->setSuffix(tr("  / 5"));
            severityBox_->setToolTip(tr("1 = minor, 5 = critical. Severity 4-5 appointments cannot be cancelled."));
            complaintEdit_ = new QLineEdit;
            complaintEdit_->setMaxLength(80);
            complaintEdit_->setPlaceholderText(tr("e.g. Severe chest pain"));
            complaintEdit_->setToolTip(tr("The main reason the patient needs emergency care."));
            form_->addRow(tr("Severity:"), severityBox_);
            form_->addRow(tr("Presenting complaint:"), complaintEdit_);
            typeNote_->setText(tr("Emergency patients can be booked any day at any time (priority: High)."));
            break;
        case PatientCategory::FollowUp:
            diagnosisEdit_ = new QLineEdit;
            diagnosisEdit_->setMaxLength(80);
            diagnosisEdit_->setPlaceholderText(tr("e.g. Malaria"));
            diagnosisEdit_->setToolTip(tr("Diagnosis from the earlier visit that is being followed up."));
            lastVisitEdit_ = new QDateEdit(QDate::currentDate().addDays(-7));
            lastVisitEdit_->setCalendarPopup(true);
            lastVisitEdit_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
            lastVisitEdit_->setMaximumDate(QDate::currentDate());
            lastVisitEdit_->setToolTip(tr("Date of the previous visit. Follow-ups must be booked after it."));
            form_->addRow(tr("Previous diagnosis:"), diagnosisEdit_);
            form_->addRow(tr("Last visit date:"), lastVisitEdit_);
            typeNote_->setText(tr("Follow-up appointments must be after the last visit, Monday to Friday, "
                                  "08:00 - 17:00 (priority: Scheduled)."));
            break;
    }
}

void PatientDialog::fillFrom(const model::PatientRecord& patient) {
    typeBox_->setCurrentIndex(typeBox_->findData(static_cast<int>(patient.category())));
    rebuildTypeFields(patient.category());   // explicit: the signal does not fire if the index is unchanged
    idEdit_->setText(qs(patient.id()));
    nameEdit_->setText(qs(patient.name()));
    ageEdit_->setText(QString::number(patient.age()));
    contactEdit_->setText(qs(patient.contact()));

    if (const auto* g = dynamic_cast<const model::GeneralPatient*>(&patient)) {
        membershipBox_->setCurrentText(qs(g->membership()));
    } else if (const auto* e = dynamic_cast<const model::EmergencyPatient*>(&patient)) {
        severityBox_->setValue(e->severity());
        complaintEdit_->setText(qs(e->complaint()));
    } else if (const auto* f = dynamic_cast<const model::FollowUpPatient*>(&patient)) {
        diagnosisEdit_->setText(qs(f->previousDiagnosis()));
        lastVisitEdit_->setDate(QDate::fromString(qs(f->lastVisitDate()), QStringLiteral("yyyy-MM-dd")));
    }
}

void PatientDialog::setHint(QLabel* label, const QString& text, bool ok) {
    label->setText(text);
    label->setStyleSheet(ok ? QStringLiteral("color:#2e7d32;font-weight:600;")
                            : QStringLiteral("color:#c62828;font-weight:600;"));
}

void PatientDialog::onIdEdited(const QString& text) {
    if (editing_) { idHint_->clear(); return; }
    const std::string id = ss(text.toUpper());
    if (!utils::validate::isPatientIdFormat(id)) setHint(idHint_, tr("Format: PAT-0001"), false);
    else if (manager_.isPatientIdTaken(id))     setHint(idHint_, tr("Already exists"), false);
    else                                        setHint(idHint_, tr("Available"), true);
}

void PatientDialog::onAgeEdited(const QString& text) {
    bool isNumber = false;
    const int age = text.trimmed().toInt(&isNumber);
    if (text.trimmed().isEmpty())                                   ageHint_->clear();
    else if (!isNumber)                                             setHint(ageHint_, tr("Numbers only"), false);
    else if (age < utils::validate::kMinAge || age > utils::validate::kMaxAge) setHint(ageHint_, tr("Must be 0 - 120"), false);
    else                                                            setHint(ageHint_, tr("OK"), true);
}

model::PatientData PatientDialog::collect() const {
    model::PatientData data;
    data.category = selectedCategory();
    data.id = ss(idEdit_->text().toUpper());
    data.name = ss(nameEdit_->text());
    data.age = ss(ageEdit_->text());
    data.contact = ss(contactEdit_->text());
    switch (data.category) {
        case PatientCategory::General:
            data.extras = {ss(membershipBox_->currentText())};
            break;
        case PatientCategory::Emergency:
            data.extras = {std::to_string(severityBox_->value()), ss(complaintEdit_->text())};
            break;
        case PatientCategory::FollowUp:
            data.extras = {ss(diagnosisEdit_->text()), ss(lastVisitEdit_->date().toString(QStringLiteral("yyyy-MM-dd")))};
            break;
    }
    return data;
}

void PatientDialog::accept() {
    // Validation happens in the model; any exception keeps the dialog open with a message.
    const bool ok = runGuarded(this, [this] {
        model::PatientData data = collect();
        if (!editing_ && manager_.isPatientIdTaken(data.id)) throw utils::DuplicateIdException(data.id);
        result_ = model::createPatient(data);
    });
    if (ok) QDialog::accept();
}

} // namespace scapms::gui
