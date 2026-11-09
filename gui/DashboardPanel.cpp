#include "gui/DashboardPanel.h"

#include "controller/ClinicManager.h"
#include "gui/BadgeDelegate.h"
#include "gui/GuiHelpers.h"
#include "utils/DateUtils.h"

#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

namespace {
constexpr int kMaxAlerts = 50;
}

DashboardPanel::DashboardPanel(controller::ClinicManager& manager, QWidget* parent)
    : Panel(manager, parent) {
    setObjectName(QStringLiteral("dashboardPanel"));

    // Header: title on the left, live clock on the right.
    auto* title = new QLabel(tr("Clinic Dashboard"));
    title->setObjectName(QStringLiteral("pageTitle"));
    auto* subtitle = new QLabel(tr("Live overview - refreshed automatically every few seconds"));
    subtitle->setObjectName(QStringLiteral("hintLabel"));
    auto* titleBox = new QVBoxLayout;
    titleBox->addWidget(title);
    titleBox->addWidget(subtitle);

    clockLabel_ = new QLabel;
    clockLabel_->setObjectName(QStringLiteral("clockLabel"));
    dateLabel_ = new QLabel;
    dateLabel_->setObjectName(QStringLiteral("hintLabel"));
    auto* clockBox = new QVBoxLayout;
    clockBox->addWidget(clockLabel_, 0, Qt::AlignRight);
    clockBox->addWidget(dateLabel_, 0, Qt::AlignRight);

    auto* header = new QHBoxLayout;
    header->addLayout(titleBox);
    header->addStretch();
    header->addLayout(clockBox);

    // Statistic cards in a 2 x 4 grid.
    cards_ = new QGridLayout;
    cards_->setSpacing(12);
    addCard(0, 0, QStringLiteral("patients"), tr("Registered patients"), QStringLiteral("#1E63B5"));
    addCard(0, 1, QStringLiteral("general"), tr("General"), QStringLiteral("#1E63B5"));
    addCard(0, 2, QStringLiteral("emergency"), tr("Emergency"), QStringLiteral("#C62828"));
    addCard(0, 3, QStringLiteral("followup"), tr("Follow-Up"), QStringLiteral("#6A3DB8"));
    addCard(1, 0, QStringLiteral("today"), tr("Appointments today"), QStringLiteral("#00796B"));
    addCard(1, 1, QStringLiteral("pending"), tr("Pending (all dates)"), QStringLiteral("#C77C02"));
    addCard(1, 2, QStringLiteral("queue"), tr("Waiting in queue"), QStringLiteral("#00796B"));
    addCard(1, 3, QStringLiteral("next"), tr("Next to be called"), QStringLiteral("#455A64"));

    // Today's schedule and the alert feed side by side.
    auto* todayBox = new QGroupBox(tr("Today's Appointments"));
    todayTable_ = createTable({tr("Time"), tr("Appointment"), tr("Patient"), tr("Service"), tr("Status")}, todayBox);
    todayTable_->setItemDelegateForColumn(4, new BadgeDelegate(todayTable_));
    todayTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    todayTable_->setToolTip(tr("All appointments dated today, in time order."));
    auto* todayLayout = new QVBoxLayout(todayBox);
    todayLayout->addWidget(todayTable_);

    auto* alertBox = new QGroupBox(tr("Reminders && Alerts"));
    alerts_ = new QListWidget(alertBox);
    alerts_->setObjectName(QStringLiteral("alertList"));
    alerts_->setToolTip(tr("Appointment reminders and waiting-time alerts raised by the timers."));
    auto* alertLayout = new QVBoxLayout(alertBox);
    alertLayout->addWidget(alerts_);

    auto* lower = new QHBoxLayout;
    lower->addWidget(todayBox, 3);
    lower->addWidget(alertBox, 2);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(header);
    layout->addLayout(cards_);
    layout->addLayout(lower, 1);

    updateClock();
    refresh();
}

QLabel* DashboardPanel::addCard(int row, int column, const QString& key, const QString& caption,
                                const QString& accent) {
    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("statCard"));
    card->setStyleSheet(QStringLiteral("#statCard { border-left: 5px solid %1; }").arg(accent));
    auto* value = new QLabel(QStringLiteral("0"));
    value->setObjectName(QStringLiteral("cardValue"));
    auto* label = new QLabel(caption);
    label->setObjectName(QStringLiteral("cardCaption"));
    auto* layout = new QVBoxLayout(card);
    layout->addWidget(value);
    layout->addWidget(label);
    cards_->addWidget(card, row, column);
    cardValues_.insert(key, value);
    return value;
}

void DashboardPanel::setCard(const QString& key, const QString& value) {
    if (QLabel* label = cardValues_.value(key)) label->setText(value);
}

void DashboardPanel::updateClock() {
    const QDateTime now = QDateTime::currentDateTime();
    clockLabel_->setText(now.toString(QStringLiteral("HH:mm:ss")));
    dateLabel_->setText(now.toString(QStringLiteral("dddd, d MMMM yyyy")));
}

void DashboardPanel::refresh() {
    int general = 0, emergency = 0, followUp = 0;
    for (const auto& patient : manager_.patients()) {
        switch (patient->category()) {
            case model::PatientCategory::General:   ++general;   break;
            case model::PatientCategory::Emergency: ++emergency; break;
            case model::PatientCategory::FollowUp:  ++followUp;  break;
        }
    }
    int pending = 0;
    for (const auto& appointment : manager_.database().appointments()) {
        if (appointment.isPending()) ++pending;
    }

    auto today = manager_.appointments().forDate(utils::date::today());
    controller::sortItems(today, controller::SortAlgorithm::Insertion,
                          [](const model::Appointment* a, const model::Appointment* b) { return a->time() < b->time(); });

    setCard(QStringLiteral("patients"), QString::number(manager_.patients().size()));
    setCard(QStringLiteral("general"), QString::number(general));
    setCard(QStringLiteral("emergency"), QString::number(emergency));
    setCard(QStringLiteral("followup"), QString::number(followUp));
    setCard(QStringLiteral("today"), QString::number(today.size()));
    setCard(QStringLiteral("pending"), QString::number(pending));
    setCard(QStringLiteral("queue"), QString::number(manager_.queueSize()));

    const auto queue = manager_.queueRows();
    setCard(QStringLiteral("next"), queue.empty() ? tr("-") : qs(queue.front().name));

    todayTable_->setRowCount(static_cast<int>(today.size()));
    int row = 0;
    for (const auto* a : today) {
        const auto* patient = manager_.peekPatient(a->patientId());
        setRow(todayTable_, row++,
               {qs(a->time()), qs(a->id()), patient ? qs(patient->name()) : qs(a->patientId()), qs(a->service()),
                qs(a->statusName())},
               qs(a->id()));
    }
}

void DashboardPanel::addAlert(const QString& message, bool urgent) {
    auto* item = new QListWidgetItem(QTime::currentTime().toString(QStringLiteral("HH:mm")) + QStringLiteral("  ") + message);
    item->setForeground(urgent ? QColor(0xC6, 0x28, 0x28) : QColor(0x00, 0x60, 0x64));
    alerts_->insertItem(0, item);
    while (alerts_->count() > kMaxAlerts) delete alerts_->takeItem(alerts_->count() - 1);
}

} // namespace scapms::gui
