#ifndef SCAPMS_GUI_ADMINPANEL_H
#define SCAPMS_GUI_ADMINPANEL_H

#include "gui/Panel.h"

class QLabel;
class QListWidget;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

// Admin tab: Undo Last Administrative Operation (stack), the contents of the
// recent-patients array cache, and an activity log of everything done this session.
class AdminPanel : public Panel {
    Q_OBJECT
public:
    AdminPanel(controller::ClinicManager& manager, QWidget* parent);

public slots:
    void refresh() override;
    void undo();
    void logActivity(const QString& message);

private:
    QPushButton* undoButton_ = nullptr;
    QLabel* nextUndo_ = nullptr;
    QListWidget* history_ = nullptr;
    QTableWidget* cacheTable_ = nullptr;
    QLabel* cacheStats_ = nullptr;
    QListWidget* log_ = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_ADMINPANEL_H
