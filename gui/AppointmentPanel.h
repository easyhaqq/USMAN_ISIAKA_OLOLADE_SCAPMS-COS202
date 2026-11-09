#ifndef SCAPMS_GUI_APPOINTMENTPANEL_H
#define SCAPMS_GUI_APPOINTMENTPANEL_H

#include "gui/Panel.h"

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTimeEdit;

namespace scapms::gui {

// Appointments tab: a booking form (QFormLayout) beside a filterable list
// with cancel / complete / remove actions.
class AppointmentPanel : public Panel {
    Q_OBJECT
public:
    AppointmentPanel(controller::ClinicManager& manager, QWidget* parent);

public slots:
    void refresh() override;

private slots:
    void scheduleAppointment();
    void cancelAppointment();
    void completeAppointment();
    void removeAppointment();
    void onPatientChanged();
    void onSlotChanged();
    void onNotesEdited(const QString& text);
    void applyFilter();
    void updateButtons();

private:
    void reloadPatients();

    QComboBox* patientBox_ = nullptr;
    QLabel* policyLabel_ = nullptr;
    QDateEdit* dateEdit_ = nullptr;
    QTimeEdit* timeEdit_ = nullptr;
    QLabel* slotLabel_ = nullptr;
    QComboBox* serviceBox_ = nullptr;
    QComboBox* doctorBox_ = nullptr;
    QLineEdit* notesEdit_ = nullptr;
    QLabel* notesCounter_ = nullptr;

    QComboBox* statusFilter_ = nullptr;
    QLineEdit* textFilter_ = nullptr;
    QCheckBox* todayOnly_ = nullptr;
    QTableWidget* table_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QPushButton* cancelButton_ = nullptr;
    QPushButton* completeButton_ = nullptr;
    QPushButton* removeButton_ = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_APPOINTMENTPANEL_H
