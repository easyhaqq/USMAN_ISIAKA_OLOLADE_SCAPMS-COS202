#ifndef SCAPMS_GUI_REPORTPANEL_H
#define SCAPMS_GUI_REPORTPANEL_H

#include "controller/ReportGenerator.h"
#include "gui/Panel.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QTableWidget;

namespace scapms::gui {

class BadgeDelegate;

// Reports tab: pick a report, view it as a table plus summary, export it to a text file.
class ReportPanel : public Panel {
    Q_OBJECT
public:
    ReportPanel(controller::ClinicManager& manager, QWidget* parent);

public slots:
    void refresh() override;   // regenerates the current report with the latest data

private slots:
    void generate();
    void exportReport();
    void onReportChanged();

private:
    void render(const controller::Report& report);

    QComboBox* reportBox_ = nullptr;
    QDateEdit* dateEdit_ = nullptr;
    QLabel* dateLabel_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QTableWidget* table_ = nullptr;
    BadgeDelegate* badges_ = nullptr;
    QLabel* summaryLabel_ = nullptr;
    controller::Report current_;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_REPORTPANEL_H
