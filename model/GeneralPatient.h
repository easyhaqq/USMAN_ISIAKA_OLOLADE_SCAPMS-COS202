#ifndef SCAPMS_MODEL_GENERALPATIENT_H
#define SCAPMS_MODEL_GENERALPATIENT_H

#include "model/Appointable.h"
#include "model/PatientRecord.h"

namespace scapms::model {

// Routine patient (student, staff or visitor) seen during regular clinic hours.
class GeneralPatient : public PatientRecord, public Appointable {
public:
    static const std::vector<std::string>& memberships();   // Student, Staff, Visitor

    GeneralPatient(std::string id, std::string name, int age, std::string contact,
                   std::string membership);

    const std::string& membership() const { return membership_; }
    void setMembership(const std::string& membership);

    // PatientRecord
    PatientCategory category() const override { return PatientCategory::General; }
    int priorityLevel() const override { return 3; }
    std::string priorityLabel() const override { return "Normal"; }
    int estimatedConsultationMinutes() const override { return 15; }
    std::string details() const override;
    std::vector<std::string> extraFields() const override { return {membership_}; }
    std::unique_ptr<PatientRecord> clone() const override;

    // Appointable
    void scheduleAppointment(const Appointment& appointment) override;
    void cancelAppointment(const std::string& appointmentId) override;
    std::string schedulingPolicy() const override;

private:
    std::string membership_;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_GENERALPATIENT_H
