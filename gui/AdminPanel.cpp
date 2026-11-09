#include "gui/AdminPanel.h"

#include "controller/ClinicManager.h"
#include "gui/BadgeDelegate.h"
#include "gui/GuiHelpers.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QTime>
#include <QVBoxLayout>

namespace scapms::gui {

namespace {
constexpr int kMaxLogEntries = 200;
}

AdminPanel::AdminPanel(controller::ClinicManager& manager, QWidget* parent) : Panel(manager, parent) {
    setObjectName(QStringLiteral("adminPanel"));

    // ---------------------------------------------------- undo stack
    auto* undoBox = new QGroupBox(tr("Undo Last Administrative Operation"));
    undoButton_ = new QPushButton(tr("Undo Last Operation"));
    undoButton_->setObjectName(QStringLiteral("primaryButton"));
    undoButton_->setToolTip(tr("Reverse the most recent operation (Ctrl+Z). Operations are undone newest first."));
    nextUndo_ = new QLabel;
    nextUndo_->setObjectName(QStringLiteral("hintLabel"));
    nextUndo_->setWordWrap(true);
    history_ = new QListWidget;
    history_->setToolTip(tr("Contents of the undo stack. The top item is undone first."));
    auto* undoLayout = new QVBoxLayout(undoBox);
    undoLayout->addWidget(undoButton_);
    undoLayout->addWidget(nextUndo_);
    undoLayout->addWidget(new QLabel(tr("Undo stack (top = most recent):")));
    undoLayout->addWidget(history_, 1);

    // ---------------------------------------------------- array cache
    auto* cacheBox = new QGroupBox(tr("Frequently Accessed Patients (fixed array of 10)"));
    cacheTable_ = createTable({tr("Slot"), tr("Patient ID"), tr("Name"), tr("Category")}, cacheBox);
    cacheTable_->setItemDelegateForColumn(3, new BadgeDelegate(cacheTable_));
    cacheTable_->setSelectionMode(QAbstractItemView::NoSelection);
    cacheTable_->setToolTip(tr("Slot 0 is the most recently accessed record; the last slot is replaced when full."));
    cacheStats_ = new QLabel;
    cacheStats_->setObjectName(QStringLiteral("hintLabel"));
    auto* cacheLayout = new QVBoxLayout(cacheBox);
    cacheLayout->addWidget(cacheTable_, 1);
    cacheLayout->addWidget(cacheStats_);

    // ---------------------------------------------------- activity log
    auto* logBox = new QGroupBox(tr("Activity Log"));
    log_ = new QListWidget;
    log_->setToolTip(tr("Everything done during this session, newest first."));
    auto* clearLog = new QPushButton(tr("Clear Log"));
    auto* logLayout = new QVBoxLayout(logBox);
    logLayout->addWidget(log_, 1);
    logLayout->addWidget(clearLog, 0, Qt::AlignRight);

    auto* right = new QVBoxLayout;
    right->addWidget(cacheBox, 1);
    right->addWidget(logBox, 1);
    auto* layout = new QHBoxLayout(this);
    layout->addWidget(undoBox, 1);
    layout->addLayout(right, 1);

    connect(undoButton_, &QPushButton::clicked, this, &AdminPanel::undo);
    connect(clearLog, &QPushButton::clicked, log_, &QListWidget::clear);
    refresh();
}

void AdminPanel::refresh() {
    const auto history = manager_.undoHistory();
    history_->clear();
    for (std::size_t i = 0; i < history.size(); ++i) {
        history_->addItem(QStringLiteral("%1.  %2").arg(i + 1).arg(qs(history[i])));
    }
    undoButton_->setEnabled(manager_.canUndo());
    nextUndo_->setText(manager_.canUndo() ? tr("Next undo: %1").arg(qs(history.front()))
                                          : tr("Nothing to undo. Register, edit, delete, schedule, cancel, complete or "
                                               "remove something first."));

    const auto& cache = manager_.recentCache();
    cacheTable_->setRowCount(static_cast<int>(controller::RecentPatientCache::kCapacity));
    for (std::size_t slot = 0; slot < controller::RecentPatientCache::kCapacity; ++slot) {
        const model::PatientRecord* p = cache.at(slot);
        setRow(cacheTable_, static_cast<int>(slot),
               {QString::number(slot), p ? qs(p->id()) : tr("(empty)"), p ? qs(p->name()) : QString(),
                p ? qs(p->categoryName()) : QString()},
               p ? qs(p->id()) : QString());
    }
    cacheTable_->resizeColumnsToContents();
    const std::size_t lookups = cache.hits() + cache.misses();
    cacheStats_->setText(tr("%1 of %2 slots used  |  lookups: %3  |  cache hits: %4  |  misses: %5")
                             .arg(cache.size()).arg(controller::RecentPatientCache::kCapacity)
                             .arg(lookups).arg(cache.hits()).arg(cache.misses()));
}

void AdminPanel::undo() {
    runGuarded(this, [this] {
        const std::string description = manager_.undoLastOperation();
        emit statusMessage(tr("Undone: %1").arg(qs(description)));
    });
    emit dataChanged();   // refresh even if the undo failed part-way
}

void AdminPanel::logActivity(const QString& message) {
    log_->insertItem(0, QTime::currentTime().toString(QStringLiteral("HH:mm:ss")) + QStringLiteral("   ") + message);
    while (log_->count() > kMaxLogEntries) delete log_->takeItem(log_->count() - 1);
}

} // namespace scapms::gui
