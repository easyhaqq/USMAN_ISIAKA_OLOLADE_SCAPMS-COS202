#ifndef SCAPMS_GUI_QUEUEPANEL_H
#define SCAPMS_GUI_QUEUEPANEL_H

#include "gui/Panel.h"

class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

// Waiting Queue tab: patients join at the back and are called from the front (FIFO).
class QueuePanel : public Panel {
    Q_OBJECT
public:
    QueuePanel(controller::ClinicManager& manager, QWidget* parent);

    static QString formatDuration(long long seconds);   // "12:05" or "1:02:05"

public slots:
    void refresh() override;
    void updateWaitingTimes();   // called every second by the MainWindow timer

private slots:
    void addToQueue();
    void callNext();

private:
    QComboBox* patientBox_ = nullptr;
    QPushButton* callButton_ = nullptr;
    QLabel* nowServing_ = nullptr;
    QLabel* summary_ = nullptr;
    QTableWidget* table_ = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_QUEUEPANEL_H
