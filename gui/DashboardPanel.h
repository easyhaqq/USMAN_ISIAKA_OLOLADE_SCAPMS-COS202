#ifndef SCAPMS_GUI_DASHBOARDPANEL_H
#define SCAPMS_GUI_DASHBOARDPANEL_H

#include "gui/Panel.h"

#include <QHash>

class QGridLayout;
class QLabel;
class QListWidget;
class QTableWidget;

namespace scapms::gui {

// Overview tab: statistic cards (QGridLayout), today's schedule and a feed of
// reminders/alerts produced by the MainWindow timers.
class DashboardPanel : public Panel {
    Q_OBJECT
public:
    DashboardPanel(controller::ClinicManager& manager, QWidget* parent);

public slots:
    void refresh() override;
    void updateClock();
    void addAlert(const QString& message, bool urgent);

private:
    QLabel* addCard(int row, int column, const QString& key, const QString& caption, const QString& accent);
    void setCard(const QString& key, const QString& value);

    QGridLayout* cards_ = nullptr;
    QHash<QString, QLabel*> cardValues_;
    QLabel* clockLabel_ = nullptr;
    QLabel* dateLabel_ = nullptr;
    QTableWidget* todayTable_ = nullptr;
    QListWidget* alerts_ = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_DASHBOARDPANEL_H
