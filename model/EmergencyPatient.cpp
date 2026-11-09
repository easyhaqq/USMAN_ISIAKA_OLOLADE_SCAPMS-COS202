#include "model/EmergencyPatient.h"

#include "model/Appointment.h"
#include "utils/Exceptions.h"
#include "utils/Validator.h"

namespace scapms::model {

EmergencyPatient::EmergencyPatient(std::string id, std::string name, int age, std::string contact,
                                   int severity, std::string complaint)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact)) {
    setSeverity(severity);
    setComplaint(complaint);
}

void EmergencyPatient::setSeverity(int severity) {
    if (severity < kMinSeverity || severity > kMaxSeverity) {
        throw utils::ValidationException("Emergency severity must be between 1 (minor) and 5 (critical).");
    }
    severity_ = severity;
}

void EmergencyPatient::setComplaint(const std::string& complaint) {
    utils::validate::requireText("Presenting complaint", complaint);
    complaint_ = utils::validate::trimmed(complaint);
}

std::string EmergencyPatient::details() const {
    return "Severity " + std::to_string(severity_) + "/5 - " + complaint_;
}

std::vector<std::string> EmergencyPatient::extraFields() const {
    return {std::to_string(severity_), complaint_};
}

std::unique_ptr<PatientRecord> EmergencyPatient::clone() const {
    return std::make_unique<EmergencyPatient>(*this);
}

void EmergencyPatient::scheduleAppointment(const Appointment& appointment) {
    // Emergency care is available every day at any hour, so no slot restriction applies.
    attachAppointmentId(appointment.id());
}

void EmergencyPatient::cancelAppointment(const std::string& appointmentId) {
    if (!hasAppointment(appointmentId)) {
        throw utils::NotFoundException("Appointment " + appointmentId + " does not belong to " + id() + ".");
    }
    if (isCritical()) {
        throw utils::AppointmentException(
            "Critical emergency appointments (severity 4-5) cannot be cancelled. "
            "Mark the appointment as completed once the patient has been seen.");
    }
    detachAppointmentId(appointmentId);
}

std::string EmergencyPatient::schedulingPolicy() const {
    return "Emergency patient: any day, any time (24/7)." +
           std::string(isCritical() ? " Critical case - cannot be cancelled." : "");
}

} // namespace scapms::model
