#ifndef SCAPMS_MODEL_EMERGENCYPATIENT_H
#define SCAPMS_MODEL_EMERGENCYPATIENT_H

#include "model/Appointable.h"
#include "model/PatientRecord.h"

namespace scapms::model {

// Urgent case. Severity runs from 1 (minor) to 5 (critical).
// Emergency care runs 24/7, and critical cases (severity >= 4) cannot be cancelled.
class EmergencyPatient : public PatientRecord, public Appointable {
public:
    static constexpr int kMinSeverity = 1;
    static constexpr int kMaxSeverity = 5;
    static constexpr int kCriticalSeverity = 4;

    EmergencyPatient(std::string id, std::string name, int age, std::string contact,
                     int severity, std::string complaint);

    int severity() const { return severity_; }
    const std::string& complaint() const { return complaint_; }
    bool isCritical() const { return severity_ >= kCriticalSeverity; }
    void setSeverity(int severity);
    void setComplaint(const std::string& complaint);

    // PatientRecord
    PatientCategory category() const override { return PatientCategory::Emergency; }
    int priorityLevel() const override { return 1; }
    std::string priorityLabel() const override { return "High Priority"; }
    int estimatedConsultationMinutes() const override { return 20 + 5 * severity_; }
    std::string details() const override;
    std::vector<std::string> extraFields() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    // Appointable
    void scheduleAppointment(const Appointment& appointment) override;
    void cancelAppointment(const std::string& appointmentId) override;
    std::string schedulingPolicy() const override;

private:
    int severity_ = kMinSeverity;
    std::string complaint_;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_EMERGENCYPATIENT_H
