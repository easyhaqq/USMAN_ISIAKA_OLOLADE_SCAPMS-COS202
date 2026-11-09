#include "gui/QueuePanel.h"

#include "controller/ClinicManager.h"
#include "gui/BadgeDelegate.h"
#include "gui/GuiHelpers.h"
#include "utils/Exceptions.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace scapms::gui {

namespace {
enum Column { ColPosition, ColId, ColName, ColCategory, ColJoined, ColWaiting, ColEstimate };
constexpr long long kLongWaitSeconds = 15 * 60;
}

QString QueuePanel::formatDuration(long long seconds) {
    const long long h = seconds / 3600, m = (seconds % 3600) / 60, s = seconds % 60;
    return h > 0 ? QStringLiteral("%1:%2:%3").arg(h).arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'))
                 : QStringLiteral("%1:%2").arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
}

QueuePanel::QueuePanel(controller::ClinicManager& manager, QWidget* parent) : Panel(manager, parent) {
    setObjectName(QStringLiteral("queuePanel"));

    patientBox_ = new QComboBox;
    patientBox_->setMinimumWidth(320);
    patientBox_->setToolTip(tr("Patient arriving at the clinic."));
    auto* addButton = new QPushButton(tr("Add to Queue"));
    addButton->setToolTip(tr("Add the chosen patient to the back of the queue."));
    callButton_ = new QPushButton(tr("Call Next Patient"));
    callButton_->setObjectName(QStringLiteral("primaryButton"));
    callButton_->setToolTip(tr("Serve the patient at the front of the queue (first in, first out)."));

    auto* controls = new QHBoxLayout;
    controls->addWidget(new QLabel(tr("Patient:")));
    controls->addWidget(patientBox_);
    controls->addWidget(addButton);
    controls->addStretch();
    controls->addWidget(callButton_);

    auto* servingFrame = new QFrame;
    servingFrame->setObjectName(QStringLiteral("servingFrame"));
    nowServing_ = new QLabel(tr("Now serving: -"));
    nowServing_->setObjectName(QStringLiteral("servingLabel"));
    summary_ = new QLabel;
    summary_->setObjectName(QStringLiteral("hintLabel"));
    auto* servingLayout = new QHBoxLayout(servingFrame);
    servingLayout->addWidget(nowServing_);
    servingLayout->addStretch();
    servingLayout->addWidget(summary_);

    table_ = createTable({tr("#"), tr("Patient ID"), tr("Name"), tr("Category"), tr("Joined"), tr("Waiting"),
                          tr("Est. Start")}, this);
    table_->setObjectName(QStringLiteral("queueTable"));
    table_->setItemDelegateForColumn(ColCategory, new BadgeDelegate(table_));
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    table_->setToolTip(tr("Front of the queue at the top. Waiting times over 15 minutes are highlighted."));

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(controls);
    layout->addWidget(servingFrame);
    layout->addWidget(table_, 1);

    connect(addButton, &QPushButton::clicked, this, &QueuePanel::addToQueue);
    connect(callButton_, &QPushButton::clicked, this, &QueuePanel::callNext);
    refresh();
}

void QueuePanel::refresh() {
    const QString current = patientBox_->currentData().toString();
    patientBox_->clear();
    for (const auto& p : manager_.patients()) {
        patientBox_->addItem(QStringLiteral("%1  -  %2").arg(qs(p->id()), qs(p->name())), qs(p->id()));
    }
    const int index = patientBox_->findData(current);
    if (index >= 0) patientBox_->setCurrentIndex(index);

    const auto rows = manager_.queueRows();
    table_->setRowCount(static_cast<int>(rows.size()));
    for (int r = 0; r < static_cast<int>(rows.size()); ++r) {
        const auto& q = rows[static_cast<std::size_t>(r)];
        setRow(table_, r,
               {QString::number(q.position), qs(q.patientId), qs(q.name), qs(q.category), qs(q.joinedAt),
                formatDuration(q.waitingSeconds),
                q.estimatedStartMinutes == 0 ? tr("Next") : tr("in ~%1 min").arg(q.estimatedStartMinutes)},
               qs(q.patientId));
    }
    table_->resizeColumnsToContents();
    callButton_->setEnabled(!rows.empty());
    updateWaitingTimes();
}

void QueuePanel::updateWaitingTimes() {
    const auto rows = manager_.queueRows();
    if (static_cast<int>(rows.size()) != table_->rowCount()) return;   // a full refresh is due
    long long longest = 0;
    for (int r = 0; r < table_->rowCount(); ++r) {
        const long long waited = rows[static_cast<std::size_t>(r)].waitingSeconds;
        longest = std::max(longest, waited);
        QTableWidgetItem* item = table_->item(r, ColWaiting);
        item->setText(formatDuration(waited));
        item->setForeground(waited >= kLongWaitSeconds ? QColor(0xC6, 0x28, 0x28) : palette().color(QPalette::Text));
    }
    summary_->setText(rows.empty() ? tr("Queue is empty")
                                   : tr("%1 waiting  |  longest wait %2").arg(rows.size()).arg(formatDuration(longest)));
}

void QueuePanel::addToQueue() {
    const QString id = patientBox_->currentData().toString();
    runGuarded(this, [&] {
        if (id.isEmpty()) throw utils::ValidationException("Please register a patient first.");
        manager_.addToQueue(ss(id));
        emit statusMessage(tr("%1 joined the waiting queue (position %2).").arg(id).arg(manager_.queueSize()));
        emit dataChanged();
    });
}

void QueuePanel::callNext() {
    runGuarded(this, [&] {
        const controller::QueueEntry entry = manager_.callNextPatient();
        const model::PatientRecord* patient = manager_.findPatient(entry.patientId);
        const QString name = patient ? qs(patient->name()) : qs(entry.patientId);
        nowServing_->setText(tr("Now serving: %1 (%2)").arg(name, qs(entry.patientId)));
        emit statusMessage(tr("Called %1 after waiting %2.").arg(name, formatDuration(entry.waitingSeconds())));

        // Offer to record the consultation against today's (or an overdue) appointment.
        if (const model::Appointment* a = manager_.appointments().consultationFor(entry.patientId)) {
            const auto answer = QMessageBox::question(
                this, tr("Consultation"),
                tr("%1 has appointment %2 (%3 at %4).\n\nMark it as completed?")
                    .arg(name, qs(a->id()), qs(a->service()), qs(a->time())));
            if (answer == QMessageBox::Yes) {
                const QString appointmentId = qs(a->id());
                manager_.appointments().complete(a->id());
                emit statusMessage(tr("Appointment %1 marked as completed.").arg(appointmentId));
            }
        } else {
            QMessageBox::information(this, tr("Now Serving"),
                                     tr("Now serving %1.\nNo appointment is due today - treated as a walk-in.").arg(name));
        }
    });
    emit dataChanged();
}

} // namespace scapms::gui
