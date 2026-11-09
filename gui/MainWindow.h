#ifndef SCAPMS_GUI_MAINWINDOW_H
#define SCAPMS_GUI_MAINWINDOW_H

#include "controller/ClinicManager.h"

#include <QMainWindow>
#include <QSet>

class QAction;
class QLabel;
class QTabWidget;
class QTimer;

namespace scapms::gui {

class AdminPanel;
class AppointmentPanel;
class DashboardPanel;
class Panel;
class PatientPanel;
class QueuePanel;
class ReportPanel;
class SearchPanel;

// Application shell: owns the ClinicManager, the tabs, the menus/shortcuts,
// the timers and file handling. Business rules live in the controller layer.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    bool loadFile(const QString& path, bool quiet);
    void loadDefaultData();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void exportPatients();
    void undo();
    void focusSearch();
    void showShortcuts();
    void showAbout();
    void refreshAll();
    void showStatus(const QString& message);
    void onClockTick();       // every second: clock, waiting times
    void checkAlerts();       // every 30 s: reminders, long waits, dashboard statistics

private:
    void createMenus();
    void createStatusBar();
    void createTimers();
    void connectPanel(Panel* panel);
    bool maybeSave();
    void updateTitle();
    void resetAlerts();
    QAction* addAction(QMenu* menu, const QString& text, const QString& shortcut, const QString& tip);

    controller::ClinicManager manager_;
    QString currentPath_;

    QTabWidget* tabs_ = nullptr;
    DashboardPanel* dashboard_ = nullptr;
    PatientPanel* patients_ = nullptr;
    AppointmentPanel* appointments_ = nullptr;
    QueuePanel* queue_ = nullptr;
    SearchPanel* search_ = nullptr;
    ReportPanel* reports_ = nullptr;
    AdminPanel* admin_ = nullptr;

    QAction* undoAction_ = nullptr;
    QLabel* patientsStatus_ = nullptr;
    QLabel* queueStatus_ = nullptr;
    QLabel* orderStatus_ = nullptr;
    QLabel* clockStatus_ = nullptr;

    QTimer* clockTimer_ = nullptr;
    QTimer* alertTimer_ = nullptr;
    QSet<QString> remindedAppointments_;   // avoid repeating the same reminder
    QSet<QString> waitAlerted_;            // avoid repeating the same waiting alert
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_MAINWINDOW_H
