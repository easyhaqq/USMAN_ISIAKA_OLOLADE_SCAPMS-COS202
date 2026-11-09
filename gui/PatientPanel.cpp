#include "gui/PatientPanel.h"

#include "controller/ClinicManager.h"
#include "controller/PatientProcessor.h"
#include "gui/BadgeDelegate.h"
#include "gui/GuiHelpers.h"
#include "gui/PatientDialog.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

namespace {
enum Column { ColId, ColName, ColAge, ColContact, ColCategory, ColPriority, ColDetails, ColAppointments, ColNext };
}

PatientPanel::PatientPanel(controller::ClinicManager& manager, QWidget* parent) : Panel(manager, parent) {
    setObjectName(QStringLiteral("patientPanel"));

    auto* addButton = new QPushButton(tr("Add Patient"));
    addButton->setObjectName(QStringLiteral("primaryButton"));
    addButton->setToolTip(tr("Register a new patient (Ctrl+N)."));
    editButton_ = new QPushButton(tr("Edit"));
    editButton_->setToolTip(tr("Edit the selected patient (or double-click a row)."));
    viewButton_ = new QPushButton(tr("View Details"));
    viewButton_->setToolTip(tr("Show the full record and appointment history of the selected patient."));
    queueButton_ = new QPushButton(tr("Send to Queue"));
    queueButton_->setToolTip(tr("Add the selected patient to the waiting queue."));
    deleteButton_ = new QPushButton(tr("Delete"));
    deleteButton_->setObjectName(QStringLiteral("dangerButton"));
    deleteButton_->setToolTip(tr("Delete the selected patient and their appointments (Delete key). Can be undone."));
    deleteButton_->setShortcut(QKeySequence::Delete);

    filterEdit_ = new QLineEdit;
    filterEdit_->setPlaceholderText(tr("Quick filter by ID or name..."));
    filterEdit_->setClearButtonEnabled(true);
    filterEdit_->setToolTip(tr("Hides rows that do not match as you type."));
    filterEdit_->setMaximumWidth(280);

    auto* toolbar = new QHBoxLayout;
    toolbar->addWidget(addButton);
    toolbar->addWidget(editButton_);
    toolbar->addWidget(viewButton_);
    toolbar->addWidget(queueButton_);
    toolbar->addWidget(deleteButton_);
    toolbar->addStretch();
    toolbar->addWidget(filterEdit_);

    table_ = createTable({tr("Patient ID"), tr("Name"), tr("Age"), tr("Contact"), tr("Category"), tr("Priority"),
                          tr("Details"), tr("Appts"), tr("Next Appointment")}, this);
    table_->setObjectName(QStringLiteral("patientTable"));
    auto* badges = new BadgeDelegate(table_);
    table_->setItemDelegateForColumn(ColCategory, badges);
    table_->setItemDelegateForColumn(ColPriority, badges);

    orderLabel_ = new QLabel;
    orderLabel_->setObjectName(QStringLiteral("hintLabel"));
    countLabel_ = new QLabel;
    countLabel_->setObjectName(QStringLiteral("hintLabel"));
    auto* footer = new QHBoxLayout;
    footer->addWidget(countLabel_);
    footer->addStretch();
    footer->addWidget(orderLabel_);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(table_, 1);
    layout->addLayout(footer);

    connect(addButton, &QPushButton::clicked, this, &PatientPanel::addPatient);
    connect(editButton_, &QPushButton::clicked, this, &PatientPanel::editPatient);
    connect(viewButton_, &QPushButton::clicked, this, &PatientPanel::viewPatient);
    connect(queueButton_, &QPushButton::clicked, this, &PatientPanel::sendToQueue);
    connect(deleteButton_, &QPushButton::clicked, this, &PatientPanel::deletePatient);
    connect(table_, &QTableWidget::cellDoubleClicked, this, [this] { editPatient(); });
    connect(table_, &QTableWidget::itemSelectionChanged, this, &PatientPanel::updateButtons);
    connect(filterEdit_, &QLineEdit::textChanged, this, &PatientPanel::applyFilter);

    refresh();
}

void PatientPanel::refresh() {
    const QString selected = selectedKey(table_);
    const auto& patients = manager_.patients();
    table_->setRowCount(static_cast<int>(patients.size()));
    for (int row = 0; row < static_cast<int>(patients.size()); ++row) {
        const auto& p = *patients[static_cast<std::size_t>(row)];
        const std::string next = manager_.database().nextAppointmentDate(p.id());
        setRow(table_, row,
               {qs(p.id()), qs(p.name()), QString::number(p.age()), qs(p.contact()), qs(p.categoryName()),
                qs(p.priorityLabel()), qs(p.details()), QString::number(p.appointmentIds().size()),
                next.empty() ? tr("-") : qs(next)},
               qs(p.id()));
        table_->item(row, ColAge)->setTextAlignment(Qt::AlignCenter);
        table_->item(row, ColAppointments)->setTextAlignment(Qt::AlignCenter);
        if (qs(p.id()) == selected) table_->selectRow(row);
    }
    table_->resizeColumnsToContents();
    applyFilter(filterEdit_->text());
    orderLabel_->setText(tr("Current order: %1").arg(qs(model::sortKeyToString(manager_.sortedBy()))));
    updateButtons();
}

void PatientPanel::applyFilter(const QString& text) {
    const QString needle = text.trimmed();
    int visible = 0;
    for (int row = 0; row < table_->rowCount(); ++row) {
        const bool match = needle.isEmpty() || table_->item(row, ColId)->text().contains(needle, Qt::CaseInsensitive) ||
                           table_->item(row, ColName)->text().contains(needle, Qt::CaseInsensitive);
        table_->setRowHidden(row, !match);
        if (match) ++visible;
    }
    countLabel_->setText(tr("Showing %1 of %2 patients").arg(visible).arg(table_->rowCount()));
}

void PatientPanel::updateButtons() {
    const bool hasSelection = !selectedKey(table_).isEmpty();
    editButton_->setEnabled(hasSelection);
    viewButton_->setEnabled(hasSelection);
    queueButton_->setEnabled(hasSelection);
    deleteButton_->setEnabled(hasSelection);
}

void PatientPanel::addPatient() {
    PatientDialog dialog(manager_, this);
    if (dialog.exec() != QDialog::Accepted) return;
    model::PatientPtr patient = dialog.takePatient();
    runGuarded(this, [&] {
        const auto& added = manager_.registerPatient(std::move(patient));
        emit statusMessage(tr("Registered %1 (%2) as a %3 patient.")
                               .arg(qs(added.name()), qs(added.id()), qs(added.categoryName())));
        emit dataChanged();
    });
}

void PatientPanel::editPatient() {
    const QString id = selectedKey(table_);
    if (id.isEmpty()) return;
    const model::PatientRecord* patient = manager_.findPatient(ss(id));
    if (!patient) return;

    PatientDialog dialog(manager_, *patient, this);
    if (dialog.exec() != QDialog::Accepted) return;
    model::PatientPtr updated = dialog.takePatient();
    runGuarded(this, [&] {
        manager_.updatePatient(std::move(updated));
        emit statusMessage(tr("Updated patient %1.").arg(id));
        emit dataChanged();
    });
}

void PatientPanel::deletePatient() {
    const QString id = selectedKey(table_);
    const model::PatientRecord* patient = id.isEmpty() ? nullptr : manager_.peekPatient(ss(id));
    if (!patient) return;

    const auto answer = QMessageBox::question(
        this, tr("Delete Patient"),
        tr("Delete %1 (%2) and their %3 appointment record(s)?\n\nYou can reverse this with Undo (Ctrl+Z).")
            .arg(qs(patient->name()), id)
            .arg(manager_.database().appointmentsFor(patient->id()).size()));
    if (answer != QMessageBox::Yes) return;

    runGuarded(this, [&] {
        manager_.deletePatient(ss(id));
        emit statusMessage(tr("Deleted patient %1.").arg(id));
        emit dataChanged();
    });
}

void PatientPanel::viewPatient() {
    const QString id = selectedKey(table_);
    const model::PatientRecord* patient = id.isEmpty() ? nullptr : manager_.findPatient(ss(id));
    if (!patient) return;

    const controller::TriageInfo info = controller::processPatient(*patient);
    QString history;
    for (const auto* a : manager_.database().appointmentsFor(patient->id())) {
        history += QStringLiteral("<li>%1 %2 - %3 with %4 <b>[%5]</b></li>")
                       .arg(qs(a->date()), qs(a->time()), qs(a->service()), qs(a->doctor()), qs(a->statusName()));
    }
    if (history.isEmpty()) history = tr("<li>No appointments yet.</li>");

    QMessageBox box(this);
    box.setWindowTitle(tr("Patient %1").arg(id));
    box.setTextFormat(Qt::RichText);
    box.setText(tr("<h3>%1</h3>"
                   "<p><b>ID:</b> %2<br><b>Age:</b> %3<br><b>Contact:</b> %4<br>"
                   "<b>Category:</b> %5 (%6)<br><b>Details:</b> %7<br>"
                   "<b>Est. consultation:</b> %8 min<br><b>Scheduling:</b> %9</p>"
                   "<p><b>Appointments</b></p><ul>%10</ul>")
                    .arg(qs(info.name), qs(info.id), QString::number(patient->age()), qs(patient->contact()),
                         qs(info.category), qs(info.priorityLabel), qs(info.details),
                         QString::number(info.consultationMinutes), qs(info.schedulingPolicy))
                    .arg(history));
    box.exec();
}

void PatientPanel::sendToQueue() {
    const QString id = selectedKey(table_);
    if (id.isEmpty()) return;
    runGuarded(this, [&] {
        manager_.addToQueue(ss(id));
        emit statusMessage(tr("%1 joined the waiting queue (position %2).").arg(id).arg(manager_.queueSize()));
        emit dataChanged();
    });
}

} // namespace scapms::gui
