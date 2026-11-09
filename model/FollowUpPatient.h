#ifndef SCAPMS_MODEL_FOLLOWUPPATIENT_H
#define SCAPMS_MODEL_FOLLOWUPPATIENT_H

#include "model/Appointable.h"
#include "model/PatientRecord.h"

namespace scapms::model {

// Returning patient reviewed after an earlier diagnosis.
// A follow-up must be booked after the last visit, within regular clinic hours.
class FollowUpPatient : public PatientRecord, public Appointable {
public:
    FollowUpPatient(std::string id, std::string name, int age, std::string contact,
                    std::string previousDiagnosis, std::string lastVisitDate);

    const std::string& previousDiagnosis() const { return previousDiagnosis_; }
    const std::string& lastVisitDate() const { return lastVisitDate_; }
    void setPreviousDiagnosis(const std::string& diagnosis);
    void setLastVisitDate(const std::string& date);

    // PatientRecord
    PatientCategory category() const override { return PatientCategory::FollowUp; }
    int priorityLevel() const override { return 2; }
    std::string priorityLabel() const override { return "Scheduled"; }
    int estimatedConsultationMinutes() const override { return 20; }
    std::string details() const override;
    std::vector<std::string> extraFields() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    // Appointable
    void scheduleAppointment(const Appointment& appointment) override;
    void cancelAppointment(const std::string& appointmentId) override;
    std::string schedulingPolicy() const override;

private:
    std::string previousDiagnosis_;
    std::string lastVisitDate_;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_FOLLOWUPPATIENT_H
