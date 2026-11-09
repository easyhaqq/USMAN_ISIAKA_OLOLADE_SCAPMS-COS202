#ifndef SCAPMS_GUI_PANEL_H
#define SCAPMS_GUI_PANEL_H

#include <QWidget>

namespace scapms::controller { class ClinicManager; }

namespace scapms::gui {

// Common base for every tab. Panels talk to the controller only, and tell the
// MainWindow about changes through signals (they never call each other).
class Panel : public QWidget {
    Q_OBJECT
public:
    Panel(controller::ClinicManager& manager, QWidget* parent) : QWidget(parent), manager_(manager) {}

public slots:
    virtual void refresh() = 0;   // redraw from the current data

signals:
    void dataChanged();                          // data was modified: refresh every panel
    void statusMessage(const QString& message);  // shown in the status bar and activity log

protected:
    controller::ClinicManager& manager_;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_PANEL_H
