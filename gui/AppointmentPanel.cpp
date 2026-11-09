#include "gui/AppointmentPanel.h"

#include "controller/ClinicManager.h"
#include "gui/BadgeDelegate.h"
#include "gui/GuiHelpers.h"
#include "model/Appointment.h"
#include "utils/DateUtils.h"
#include "utils/Exceptions.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace scapms::gui {

namespace {
enum Column { ColId, ColPatient, ColName, ColDate, ColDay, ColTime, ColService, ColDoctor, ColStatus, ColNotes };
constexpr int kMaxNotes = 80;
}

AppointmentPanel::AppointmentPanel(controller::ClinicManager& manager, QWidget* parent) : Panel(manager, parent) {
    setObjectName(QStringLiteral("appointmentPanel"));

    // ------------------------------------------------ booking form (left)
    auto* formBox = new QGroupBox(tr("Schedule Appointment"));
    formBox->setMinimumWidth(360);
    formBox->setMaximumWidth(430);
    auto* form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setVerticalSpacing(10);

    patientBox_ = new QComboBox;
    patientBox_->setToolTip(tr("Choose the patient to book. The scheduling rules for the patient type appear below."));
    form->addRow(tr("Patient:"), patientBox_);

    policyLabel_ = new QLabel;
    policyLabel_->setObjectName(QStringLiteral("hintLabel"));
    policyLabel_->setWordWrap(true);
    form->addRow(QString(), policyLabel_);

    dateEdit_ = new QDateEdit(QDate::currentDate().addDays(1));
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat(QStringLiteral("yyyy-MM-dd  (ddd)"));
    dateEdit_->setMinimumDate(QDate::currentDate());
    dateEdit_->setToolTip(tr("Appointment date. Past dates cannot be selected."));
    form->addRow(tr("Date:"), dateEdit_);

    timeEdit_ = new QTimeEdit(QTime(9, 0));
    timeEdit_->setDisplayFormat(QStringLiteral("HH:mm"));
    timeEdit_->setToolTip(tr("Start time (24-hour clock)."));
    form->addRow(tr("Time:"), timeEdit_);

    slotLabel_ = new QLabel;
    slotLabel_->setWordWrap(true);
    form->addRow(QString(), slotLabel_);

    serviceBox_ = new QComboBox;
    for (const auto& service : model::Appointment::medicalServices()) serviceBox_->addItem(qs(service));
    serviceBox_->setToolTip(tr("Medical service being booked."));
    form->addRow(tr("Service:"), serviceBox_);

    doctorBox_ = new QComboBox;
    doctorBox_->setEditable(true);
    for (const auto& doctor : model::Appointment::doctors()) doctorBox_->addItem(qs(doctor));
    doctorBox_->setToolTip(tr("Choose a clinician or type a name. A clinician cannot be double-booked."));
    form->addRow(tr("Doctor:"), doctorBox_);

    notesEdit_ = new QLineEdit;
    notesEdit_->setMaxLength(kMaxNotes);
    notesEdit_->setPlaceholderText(tr("Optional, e.g. reason for visit"));
    notesEdit_->setToolTip(tr("Short note shown on the appointment (max %1 characters).").arg(kMaxNotes));
    notesCounter_ = new QLabel;
    notesCounter_->setObjectName(QStringLiteral("hintLabel"));
    auto* notesRow = new QHBoxLayout;
    notesRow->addWidget(notesEdit_, 1);
    notesRow->addWidget(notesCounter_);
    form->addRow(tr("Notes:"), notesRow);

    auto* scheduleButton = new QPushButton(tr("Schedule Appointment"));
    scheduleButton->setObjectName(QStringLiteral("primaryButton"));
    scheduleButton->setToolTip(tr("Book the appointment after checking all rules."));

    auto* formLayout = new QVBoxLayout(formBox);
    formLayout->addLayout(form);
    formLayout->addWidget(scheduleButton);
    formLayout->addStretch();

    // ------------------------------------------------ appointment list (right)
    statusFilter_ = new QComboBox;
    statusFilter_->addItems({tr("All statuses"), tr("Pending"), tr("Completed"), tr("Cancelled")});
    statusFilter_->setToolTip(tr("Show only appointments with this status."));
    textFilter_ = new QLineEdit;
    textFilter_->setPlaceholderText(tr("Filter by patient, name or appointment ID..."));
    textFilter_->setClearButtonEnabled(true);
    todayOnly_ = new QCheckBox(tr("Today only"));
    todayOnly_->setToolTip(tr("Show only appointments dated today."));
    auto* filters = new QHBoxLayout;
    filters->addWidget(statusFilter_);
    filters->addWidget(textFilter_, 1);
    filters->addWidget(todayOnly_);

    table_ = createTable({tr("Appointment"), tr("Patient"), tr("Name"), tr("Date"), tr("Day"), tr("Time"),
                          tr("Service"), tr("Doctor"), tr("Status"), tr("Notes")}, this);
    table_->setObjectName(QStringLiteral("appointmentTable"));
    table_->setItemDelegateForColumn(ColStatus, new BadgeDelegate(table_));

    cancelButton_ = new QPushButton(tr("Cancel Appointment"));
    cancelButton_->setToolTip(tr("Mark the selected pending appointment as cancelled (can be undone)."));
    completeButton_ = new QPushButton(tr("Mark Completed"));
    completeButton_->setToolTip(tr("Record that the patient attended the selected appointment."));
    removeButton_ = new QPushButton(tr("Remove Record"));
    removeButton_->setObjectName(QStringLiteral("dangerButton"));
    removeButton_->setToolTip(tr("Administrative: permanently remove the appointment record (can be undone)."));
    countLabel_ = new QLabel;
    countLabel_->setObjectName(QStringLiteral("hintLabel"));
    auto* actions = new QHBoxLayout;
    actions->addWidget(countLabel_);
    actions->addStretch();
    actions->addWidget(completeButton_);
    actions->addWidget(cancelButton_);
    actions->addWidget(removeButton_);

    auto* listBox = new QGroupBox(tr("Appointments"));
    auto* listLayout = new QVBoxLayout(listBox);
    listLayout->addLayout(filters);
    listLayout->addWidget(table_, 1);
    listLayout->addLayout(actions);

    auto* layout = new QHBoxLayout(this);
    layout->addWidget(formBox);
    layout->addWidget(listBox, 1);

    // ------------------------------------------------ events
    connect(scheduleButton, &QPushButton::clicked, this, &AppointmentPanel::scheduleAppointment);
    connect(cancelButton_, &QPushButton::clicked, this, &AppointmentPanel::cancelAppointment);
    connect(completeButton_, &QPushButton::clicked, this, &AppointmentPanel::completeAppointment);
    connect(removeButton_, &QPushButton::clicked, this, &AppointmentPanel::removeAppointment);
    connect(patientBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, &AppointmentPanel::onPatientChanged);
    connect(dateEdit_, &QDateEdit::dateChanged, this, &AppointmentPanel::onSlotChanged);
    connect(timeEdit_, &QTimeEdit::timeChanged, this, &AppointmentPanel::onSlotChanged);
    connect(notesEdit_, &QLineEdit::textChanged, this, &AppointmentPanel::onNotesEdited);
    connect(statusFilter_, qOverload<int>(&QComboBox::currentIndexChanged), this, &AppointmentPanel::applyFilter);
    connect(textFilter_, &QLineEdit::textChanged, this, &AppointmentPanel::applyFilter);
    connect(todayOnly_, &QCheckBox::toggled, this, &AppointmentPanel::applyFilter);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &AppointmentPanel::updateButtons);

    onNotesEdited(QString());
    refresh();
}

void AppointmentPanel::reloadPatients() {
    const QString current = patientBox_->currentData().toString();
    QSignalBlocker blocker(patientBox_);
    patientBox_->clear();
    for (const auto& p : manager_.patients()) {
        patientBox_->addItem(QStringLiteral("%1  -  %2  (%3)").arg(qs(p->id()), qs(p->name()), qs(p->categoryName())),
                             qs(p->id()));
    }
    const int index = patientBox_->findData(current);
    patientBox_->setCurrentIndex(index >= 0 ? index : 0);
}

void AppointmentPanel::refresh() {
    reloadPatients();
    onPatientChanged();

    const QString selected = selectedKey(table_);
    const auto& appointments = manager_.database().appointments();
    table_->setRowCount(static_cast<int>(appointments.size()));
    int row = 0;
    for (const auto& a : appointments) {
        const auto* patient = manager_.peekPatient(a.patientId());
        setRow(table_, row,
               {qs(a.id()), qs(a.patientId()), patient ? qs(patient->name()) : tr("(deleted)"), qs(a.date()),
                qs(utils::date::dayName(a.date())).left(3), qs(a.time()), qs(a.service()), qs(a.doctor()),
                qs(a.statusName()), qs(a.notes())},
               qs(a.id()));
        if (qs(a.id()) == selected) table_->selectRow(row);
        ++row;
    }
    table_->resizeColumnsToContents();
    applyFilter();
}

void AppointmentPanel::applyFilter() {
    const QString status = statusFilter_->currentIndex() == 0 ? QString() : statusFilter_->currentText();
    const QString needle = textFilter_->text().trimmed();
    const QString today = qs(utils::date::today());
    int visible = 0;
    for (int row = 0; row < table_->rowCount(); ++row) {
        bool match = status.isEmpty() || table_->item(row, ColStatus)->text() == status;
        match = match && (!todayOnly_->isChecked() || table_->item(row, ColDate)->text() == today);
        match = match && (needle.isEmpty() || table_->item(row, ColId)->text().contains(needle, Qt::CaseInsensitive) ||
                          table_->item(row, ColPatient)->text().contains(needle, Qt::CaseInsensitive) ||
                          table_->item(row, ColName)->text().contains(needle, Qt::CaseInsensitive));
        table_->setRowHidden(row, !match);
        if (match) ++visible;
    }
    countLabel_->setText(tr("Showing %1 of %2 appointments").arg(visible).arg(table_->rowCount()));
    updateButtons();
}

void AppointmentPanel::updateButtons() {
    const QString id = selectedKey(table_);
    const model::Appointment* a = id.isEmpty() ? nullptr : manager_.database().findAppointment(ss(id));
    const bool pending = a && a->isPending() && !table_->isRowHidden(table_->currentRow());
    cancelButton_->setEnabled(pending);
    completeButton_->setEnabled(pending);
    removeButton_->setEnabled(a != nullptr);
}

void AppointmentPanel::onPatientChanged() {
    const QString id = patientBox_->currentData().toString();
    const model::PatientRecord* patient = id.isEmpty() ? nullptr : manager_.peekPatient(ss(id));
    if (!patient) {
        policyLabel_->setText(tr("Register a patient first."));
        onSlotChanged();
        return;
    }
    policyLabel_->setText(qs(manager_.appointments().policyFor(patient->id())));

    // Suggest the usual service and clinician for the patient type.
    switch (patient->category()) {
        case model::PatientCategory::Emergency:
            serviceBox_->setCurrentText(QStringLiteral("Emergency Care"));
            doctorBox_->setCurrentText(QStringLiteral("Dr. C. Okafor (Emergency)"));
            break;
        case model::PatientCategory::FollowUp:
            serviceBox_->setCurrentText(QStringLiteral("Follow-Up Review"));
            doctorBox_->setCurrentText(QStringLiteral("Dr. A. Bello (General Practice)"));
            break;
        case model::PatientCategory::General:
            serviceBox_->setCurrentText(QStringLiteral("General Consultation"));
            doctorBox_->setCurrentText(QStringLiteral("Dr. A. Bello (General Practice)"));
            break;
    }
    onSlotChanged();
}

void AppointmentPanel::onSlotChanged() {
    const QString id = patientBox_->currentData().toString();
    const model::PatientRecord* patient = id.isEmpty() ? nullptr : manager_.peekPatient(ss(id));
    const QDate date = dateEdit_->date();
    const QTime time = timeEdit_->time();
    const bool weekend = date.dayOfWeek() >= 6;
    const bool outOfHours = time < QTime(8, 0) || time > QTime(17, 0);
    const bool emergency = patient && patient->category() == model::PatientCategory::Emergency;

    QString text;
    bool ok = true;
    if (!emergency && weekend) {
        text = tr("%1 is a weekend - only emergency patients can be booked.").arg(date.toString(QStringLiteral("dddd")));
        ok = false;
    } else if (!emergency && outOfHours) {
        text = tr("Outside clinic hours (08:00 - 17:00).");
        ok = false;
    } else if (date == QDate::currentDate() && time < QTime::currentTime()) {
        text = tr("That time has already passed today.");
        ok = false;
    } else {
        text = tr("%1 at %2").arg(date.toString(QStringLiteral("dddd d MMM")), time.toString(QStringLiteral("HH:mm")));
    }
    slotLabel_->setText(text);
    slotLabel_->setStyleSheet(ok ? QStringLiteral("color:#2e7d32;") : QStringLiteral("color:#c62828;font-weight:600;"));
}

void AppointmentPanel::onNotesEdited(const QString& text) {
    notesCounter_->setText(QStringLiteral("%1/%2").arg(text.size()).arg(kMaxNotes));
}

void AppointmentPanel::scheduleAppointment() {
    const QString patientId = patientBox_->currentData().toString();
    runGuarded(this, [&] {
        if (patientId.isEmpty()) throw utils::ValidationException("Please select a patient.");
        const std::string id = manager_.appointments().schedule(
            ss(patientId), ss(dateEdit_->date().toString(QStringLiteral("yyyy-MM-dd"))),
            ss(timeEdit_->time().toString(QStringLiteral("HH:mm"))), ss(serviceBox_->currentText()),
            ss(doctorBox_->currentText()), ss(notesEdit_->text()));
        notesEdit_->clear();
        emit statusMessage(tr("Scheduled %1 for %2 on %3 at %4.")
                               .arg(qs(id), patientId, dateEdit_->date().toString(QStringLiteral("yyyy-MM-dd")),
                                    timeEdit_->time().toString(QStringLiteral("HH:mm"))));
        emit dataChanged();
    });
}

void AppointmentPanel::cancelAppointment() {
    const QString id = selectedKey(table_);
    if (id.isEmpty()) return;
    if (QMessageBox::question(this, tr("Cancel Appointment"), tr("Cancel appointment %1?").arg(id)) != QMessageBox::Yes) {
        return;
    }
    runGuarded(this, [&] {
        manager_.appointments().cancel(ss(id));
        emit statusMessage(tr("Cancelled appointment %1.").arg(id));
        emit dataChanged();
    });
}

void AppointmentPanel::completeAppointment() {
    const QString id = selectedKey(table_);
    if (id.isEmpty()) return;
    runGuarded(this, [&] {
        manager_.appointments().complete(ss(id));
        emit statusMessage(tr("Appointment %1 marked as completed.").arg(id));
        emit dataChanged();
    });
}

void AppointmentPanel::removeAppointment() {
    const QString id = selectedKey(table_);
    if (id.isEmpty()) return;
    if (QMessageBox::question(this, tr("Remove Appointment Record"),
                              tr("Permanently remove the record of %1?\n\nYou can reverse this with Undo (Ctrl+Z).").arg(id)) !=
        QMessageBox::Yes) {
        return;
    }
    runGuarded(this, [&] {
        manager_.appointments().remove(ss(id));
        emit statusMessage(tr("Removed appointment record %1.").arg(id));
        emit dataChanged();
    });
}

} // namespace scapms::gui
