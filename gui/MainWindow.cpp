#include "gui/MainWindow.h"

#include "gui/AdminPanel.h"
#include "gui/AppointmentPanel.h"
#include "gui/DashboardPanel.h"
#include "gui/GuiHelpers.h"
#include "gui/PatientPanel.h"
#include "gui/QueuePanel.h"
#include "gui/ReportPanel.h"
#include "gui/SearchPanel.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>

namespace scapms::gui {

namespace {
constexpr int kClockIntervalMs = 1000;
constexpr int kAlertIntervalMs = 30 * 1000;
constexpr int kReminderWindowMinutes = 30;
constexpr long long kLongWaitSeconds = 15 * 60;
const QString kFileFilter = QStringLiteral("SCAPMS data (*.txt);;All files (*)");
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setObjectName(QStringLiteral("mainWindow"));
    resize(1320, 820);
    setMinimumSize(1024, 680);

    tabs_ = new QTabWidget(this);
    tabs_->setObjectName(QStringLiteral("mainTabs"));
    tabs_->setDocumentMode(true);
    dashboard_ = new DashboardPanel(manager_, tabs_);
    patients_ = new PatientPanel(manager_, tabs_);
    appointments_ = new AppointmentPanel(manager_, tabs_);
    queue_ = new QueuePanel(manager_, tabs_);
    search_ = new SearchPanel(manager_, tabs_);
    reports_ = new ReportPanel(manager_, tabs_);
    admin_ = new AdminPanel(manager_, tabs_);

    tabs_->addTab(dashboard_, tr("Dashboard"));
    tabs_->addTab(patients_, tr("Patient Records"));
    tabs_->addTab(appointments_, tr("Appointments"));
    tabs_->addTab(queue_, tr("Waiting Queue"));
    tabs_->addTab(search_, tr("Search && Sort"));
    tabs_->addTab(reports_, tr("Reports"));
    tabs_->addTab(admin_, tr("Admin"));
    setCentralWidget(tabs_);

    for (Panel* panel : std::initializer_list<Panel*>{dashboard_, patients_, appointments_, queue_, search_, reports_, admin_}) {
        connectPanel(panel);
    }

    createMenus();
    createStatusBar();
    createTimers();
    refreshAll();
}

// =============================================================== setup

QAction* MainWindow::addAction(QMenu* menu, const QString& text, const QString& shortcut, const QString& tip) {
    QAction* action = menu->addAction(text);
    if (!shortcut.isEmpty()) action->setShortcut(QKeySequence(shortcut));
    action->setStatusTip(tip);
    action->setToolTip(shortcut.isEmpty() ? tip : QStringLiteral("%1 (%2)").arg(tip, shortcut));
    return action;
}

void MainWindow::createMenus() {
    QMenu* file = menuBar()->addMenu(tr("&File"));
    QAction* newAction = addAction(file, tr("&New"), QStringLiteral("Ctrl+Shift+N"), tr("Start a new, empty clinic file"));
    QAction* openAction = addAction(file, tr("&Load..."), QStringLiteral("Ctrl+O"), tr("Load patient and appointment data"));
    QAction* saveAction = addAction(file, tr("&Save"), QStringLiteral("Ctrl+S"), tr("Save all data"));
    QAction* saveAsAction = addAction(file, tr("Save &As..."), QStringLiteral("Ctrl+Shift+S"), tr("Save the data to a new file"));
    file->addSeparator();
    QAction* exportAction = addAction(file, tr("&Export Patients (CSV)..."), QStringLiteral("Ctrl+E"),
                                      tr("Export the patient list for Excel or other programs"));
    file->addSeparator();
    QAction* exitAction = addAction(file, tr("E&xit"), QStringLiteral("Ctrl+Q"), tr("Close the application"));

    QMenu* edit = menuBar()->addMenu(tr("&Edit"));
    undoAction_ = addAction(edit, tr("&Undo Last Operation"), QStringLiteral("Ctrl+Z"), tr("Reverse the most recent administrative operation"));
    edit->addSeparator();
    QAction* addPatient = addAction(edit, tr("&Add Patient..."), QStringLiteral("Ctrl+N"), tr("Register a new patient"));
    QAction* find = addAction(edit, tr("&Find Patient"), QStringLiteral("Ctrl+F"), tr("Jump to the search box"));

    QMenu* view = menuBar()->addMenu(tr("&View"));
    for (int i = 0; i < tabs_->count(); ++i) {
        QAction* tab = addAction(view, tabs_->tabText(i).replace(QStringLiteral("&&"), QStringLiteral("&")),
                                 QStringLiteral("Ctrl+%1").arg(i + 1), tr("Open the %1 tab").arg(tabs_->tabText(i)));
        connect(tab, &QAction::triggered, this, [this, i] { tabs_->setCurrentIndex(i); });
    }

    QMenu* help = menuBar()->addMenu(tr("&Help"));
    QAction* shortcuts = addAction(help, tr("&Keyboard Shortcuts"), QStringLiteral("F1"), tr("List all keyboard shortcuts"));
    QAction* about = addAction(help, tr("&About SCAPMS"), QString(), tr("About this application"));

    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportPatients);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    connect(undoAction_, &QAction::triggered, this, &MainWindow::undo);
    connect(addPatient, &QAction::triggered, this, [this] {
        tabs_->setCurrentWidget(patients_);
        patients_->addPatient();
    });
    connect(find, &QAction::triggered, this, &MainWindow::focusSearch);
    connect(shortcuts, &QAction::triggered, this, &MainWindow::showShortcuts);
    connect(about, &QAction::triggered, this, &MainWindow::showAbout);

    // Toolbar with the most common actions.
    QToolBar* toolbar = addToolBar(tr("Main"));
    toolbar->setObjectName(QStringLiteral("mainToolbar"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    toolbar->addAction(openAction);
    toolbar->addAction(saveAction);
    toolbar->addSeparator();
    toolbar->addAction(addPatient);
    toolbar->addAction(find);
    toolbar->addAction(undoAction_);
}

void MainWindow::createStatusBar() {
    patientsStatus_ = new QLabel;
    queueStatus_ = new QLabel;
    orderStatus_ = new QLabel;
    clockStatus_ = new QLabel;
    for (QLabel* label : {patientsStatus_, queueStatus_, orderStatus_, clockStatus_}) {
        label->setObjectName(QStringLiteral("statusField"));
        statusBar()->addPermanentWidget(label);
    }
    statusBar()->showMessage(tr("Ready"), 3000);
}

void MainWindow::createTimers() {
    clockTimer_ = new QTimer(this);
    connect(clockTimer_, &QTimer::timeout, this, &MainWindow::onClockTick);
    clockTimer_->start(kClockIntervalMs);

    alertTimer_ = new QTimer(this);
    connect(alertTimer_, &QTimer::timeout, this, &MainWindow::checkAlerts);
    alertTimer_->start(kAlertIntervalMs);
    QTimer::singleShot(1500, this, &MainWindow::checkAlerts);   // first check shortly after start-up
}

void MainWindow::connectPanel(Panel* panel) {
    connect(panel, &Panel::dataChanged, this, &MainWindow::refreshAll);
    connect(panel, &Panel::statusMessage, this, &MainWindow::showStatus);
}

// ============================================================ refreshing

void MainWindow::refreshAll() {
    for (Panel* panel : std::initializer_list<Panel*>{dashboard_, patients_, appointments_, queue_, search_, reports_, admin_}) {
        panel->refresh();
    }
    undoAction_->setEnabled(manager_.canUndo());
    undoAction_->setIconText(tr("Undo"));   // short label on the toolbar, full text in the menu
    undoAction_->setText(manager_.canUndo() ? tr("&Undo: %1").arg(qs(manager_.undoHistory().front()))
                                            : tr("&Undo Last Operation"));
    patientsStatus_->setText(tr("Patients: %1").arg(manager_.patients().size()));
    queueStatus_->setText(tr("Waiting: %1").arg(manager_.queueSize()));
    orderStatus_->setText(tr("Order: %1").arg(qs(model::sortKeyToString(manager_.sortedBy()))));
    updateTitle();
}

void MainWindow::showStatus(const QString& message) {
    statusBar()->showMessage(message, 6000);
    admin_->logActivity(message);
}

void MainWindow::updateTitle() {
    const QString name = currentPath_.isEmpty() ? tr("Untitled") : QFileInfo(currentPath_).fileName();
    setWindowTitle(tr("SCAPMS - Smart Clinic Appointment & Patient Management - %1%2")
                       .arg(name, manager_.hasUnsavedChanges() ? QStringLiteral(" *") : QString()));
}

// ================================================================ timers

void MainWindow::onClockTick() {
    dashboard_->updateClock();
    queue_->updateWaitingTimes();
    clockStatus_->setText(QDateTime::currentDateTime().toString(QStringLiteral("ddd d MMM  HH:mm:ss")));
}

void MainWindow::checkAlerts() {
    // Appointment reminders: pending appointments today starting within the window.
    for (const auto* a : manager_.appointments().dueSoon(kReminderWindowMinutes)) {
        const QString id = qs(a->id());
        if (remindedAppointments_.contains(id)) continue;
        remindedAppointments_.insert(id);
        const auto* patient = manager_.peekPatient(a->patientId());
        const QString text = tr("Reminder: %1 (%2) at %3 - %4")
                                 .arg(patient ? qs(patient->name()) : qs(a->patientId()), id, qs(a->time()), qs(a->service()));
        dashboard_->addAlert(text, false);
        showStatus(text);
    }

    // Waiting-time alerts: anyone waiting longer than the threshold.
    for (const auto& row : manager_.queueRows()) {
        const QString key = qs(row.patientId) + QLatin1Char('@') + qs(row.joinedAt);
        if (row.waitingSeconds < kLongWaitSeconds || waitAlerted_.contains(key)) continue;
        waitAlerted_.insert(key);
        const QString text = tr("Alert: %1 has been waiting %2 minutes").arg(qs(row.name)).arg(row.waitingSeconds / 60);
        dashboard_->addAlert(text, true);
        showStatus(text);
    }

    dashboard_->refresh();   // dashboard statistics refresh
}

void MainWindow::resetAlerts() {
    remindedAppointments_.clear();
    waitAlerted_.clear();
}

// ========================================================== file handling

bool MainWindow::maybeSave() {
    if (!manager_.hasUnsavedChanges()) return true;
    const auto answer = QMessageBox::warning(this, tr("Unsaved Changes"),
                                             tr("The clinic data has been modified.\nDo you want to save your changes?"),
                                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) return saveFile();
    return answer == QMessageBox::Discard;
}

void MainWindow::newFile() {
    if (!maybeSave()) return;
    manager_.reset();
    currentPath_.clear();
    resetAlerts();
    showStatus(tr("Started a new, empty clinic file."));
    refreshAll();
}

bool MainWindow::loadFile(const QString& path, bool quiet) {
    std::vector<std::string> warnings;
    const bool ok = runGuarded(this, [&] { warnings = manager_.load(ss(path)); });
    if (!ok) return false;

    currentPath_ = path;
    resetAlerts();
    refreshAll();
    showStatus(tr("Loaded %1 patients and %2 appointments from %3")
                   .arg(manager_.patients().size()).arg(manager_.database().appointments().size())
                   .arg(QFileInfo(path).fileName()));
    if (!warnings.empty() && !quiet) {
        QStringList lines;
        for (const auto& w : warnings) lines << qs(w);
        QMessageBox::warning(this, tr("Some Records Were Skipped"),
                             tr("%1 line(s) could not be loaded:\n\n%2").arg(warnings.size()).arg(lines.join('\n')));
    }
    return true;
}

void MainWindow::loadDefaultData() {
    // Look for data/clinic_data.txt beside the executable, then in the working folder.
    const QStringList candidates = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/data/clinic_data.txt"),
        QDir::currentPath() + QStringLiteral("/data/clinic_data.txt"),
        QDir::currentPath() + QStringLiteral("/../data/clinic_data.txt"),
    };
    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            loadFile(QFileInfo(path).canonicalFilePath(), true);
            return;
        }
    }
    showStatus(tr("No sample data found - starting with an empty clinic. Use File > Load to open a data file."));
}

void MainWindow::openFile() {
    if (!maybeSave()) return;
    const QString start = currentPath_.isEmpty() ? QDir::currentPath() : QFileInfo(currentPath_).absolutePath();
    const QString path = QFileDialog::getOpenFileName(this, tr("Load Clinic Data"), start, kFileFilter);
    if (!path.isEmpty()) loadFile(path, false);
}

bool MainWindow::saveFile() {
    if (currentPath_.isEmpty()) return saveFileAs();
    const bool ok = runGuarded(this, [&] { manager_.save(ss(currentPath_)); });
    if (ok) showStatus(tr("Saved to %1").arg(currentPath_));
    refreshAll();
    return ok;
}

bool MainWindow::saveFileAs() {
    const QString start = currentPath_.isEmpty() ? QDir::currentPath() + QStringLiteral("/clinic_data.txt") : currentPath_;
    QString path = QFileDialog::getSaveFileName(this, tr("Save Clinic Data As"), start, kFileFilter);
    if (path.isEmpty()) return false;
    if (QFileInfo(path).suffix().isEmpty()) path += QStringLiteral(".txt");
    const QString previous = currentPath_;
    currentPath_ = path;
    if (saveFile()) return true;
    currentPath_ = previous;   // keep the old file name if the new location could not be written
    updateTitle();
    return false;
}

void MainWindow::exportPatients() {
    const QString path = QFileDialog::getSaveFileName(this, tr("Export Patients"), QDir::currentPath() + QStringLiteral("/patients.csv"),
                                                      tr("CSV files (*.csv);;All files (*)"));
    if (path.isEmpty()) return;
    if (runGuarded(this, [&] { manager_.exportPatientsCsv(ss(path)); })) {
        showStatus(tr("Exported %1 patients to %2").arg(manager_.patients().size()).arg(path));
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (maybeSave()) event->accept();
    else event->ignore();
}

// ================================================================ actions

void MainWindow::undo() {
    admin_->undo();
}

void MainWindow::focusSearch() {
    tabs_->setCurrentWidget(search_);
    search_->focusSearch();
}

void MainWindow::showShortcuts() {
    QMessageBox::information(this, tr("Keyboard Shortcuts"),
                             tr("Ctrl+N\tAdd patient\n"
                                "Ctrl+F\tFind patient\n"
                                "Ctrl+Z\tUndo last operation\n"
                                "Ctrl+S\tSave\n"
                                "Ctrl+Shift+S\tSave as\n"
                                "Ctrl+O\tLoad\n"
                                "Ctrl+E\tExport patients (CSV)\n"
                                "Ctrl+Shift+N\tNew file\n"
                                "Ctrl+1 ... Ctrl+7\tSwitch tab\n"
                                "Delete\tDelete selected patient\n"
                                "Ctrl+Q\tExit"));
}

void MainWindow::showAbout() {
    QMessageBox::about(this, tr("About SCAPMS"),
                       tr("<h3>Smart Clinic Appointment &amp; Patient Management System</h3>"
                          "<p>COS202 Programming II - Lab Assessment Project.</p>"
                          "<p>Built with C++17 and Qt %1. Model, controller and utility code are "
                          "standard C++ with no GUI dependency.</p>").arg(QString::fromLatin1(qVersion())));
}

} // namespace scapms::gui
