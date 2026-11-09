#ifndef SCAPMS_GUI_PATIENTDIALOG_H
#define SCAPMS_GUI_PATIENTDIALOG_H

#include "model/PatientFactory.h"

#include <QDialog>

class QComboBox;
class QDateEdit;
class QFormLayout;
class QLabel;
class QLineEdit;
class QSpinBox;

namespace scapms::controller { class ClinicManager; }

namespace scapms::gui {

// Register / edit dialog. The type-specific rows are removed and re-added
// whenever the patient type changes (dynamic components), and the ID and age
// fields are validated live as the user types (text-field events).
class PatientDialog : public QDialog {
    Q_OBJECT
public:
    PatientDialog(controller::ClinicManager& manager, QWidget* parent);                    // register
    PatientDialog(controller::ClinicManager& manager, const model::PatientRecord& existing,
                  QWidget* parent);                                                         // edit

    model::PatientPtr takePatient() { return std::move(result_); }

public slots:
    void accept() override;

private slots:
    void onTypeChanged(int index);
    void onIdEdited(const QString& text);
    void onAgeEdited(const QString& text);

private:
    void buildForm();
    void rebuildTypeFields(model::PatientCategory category);
    void fillFrom(const model::PatientRecord& patient);
    model::PatientData collect() const;
    model::PatientCategory selectedCategory() const;
    static void setHint(QLabel* label, const QString& text, bool ok);

    controller::ClinicManager& manager_;
    bool editing_ = false;
    model::PatientPtr result_;

    QFormLayout* form_ = nullptr;
    int baseRowCount_ = 0;
    QComboBox* typeBox_ = nullptr;
    QLineEdit* idEdit_ = nullptr;
    QLabel* idHint_ = nullptr;
    QLineEdit* nameEdit_ = nullptr;
    QLineEdit* ageEdit_ = nullptr;
    QLabel* ageHint_ = nullptr;
    QLineEdit* contactEdit_ = nullptr;
    QLabel* typeNote_ = nullptr;

    // Type-specific widgets (only those of the selected type exist at a time).
    QComboBox* membershipBox_ = nullptr;
    QSpinBox* severityBox_ = nullptr;
    QLineEdit* complaintEdit_ = nullptr;
    QLineEdit* diagnosisEdit_ = nullptr;
    QDateEdit* lastVisitEdit_ = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_PATIENTDIALOG_H
