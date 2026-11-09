#ifndef SCAPMS_GUI_PATIENTPANEL_H
#define SCAPMS_GUI_PATIENTPANEL_H

#include "gui/Panel.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

// Patient Records tab: view, register, edit, delete and send patients to the queue.
class PatientPanel : public Panel {
    Q_OBJECT
public:
    PatientPanel(controller::ClinicManager& manager, QWidget* parent);

public slots:
    void refresh() override;
    void addPatient();
    void editPatient();
    void deletePatient();
    void viewPatient();
    void sendToQueue();

private slots:
    void applyFilter(const QString& text);
    void updateButtons();

private:
    QTableWidget* table_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    QLabel* orderLabel_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QPushButton* editButton_ = nullptr;
    QPushButton* viewButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
    QPushButton* queueButton_ = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_PATIENTPANEL_H
