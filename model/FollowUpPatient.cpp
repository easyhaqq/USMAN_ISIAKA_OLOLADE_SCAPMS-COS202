#include "model/FollowUpPatient.h"

#include "model/Appointment.h"
#include "model/SchedulingRules.h"
#include "utils/DateUtils.h"
#include "utils/Exceptions.h"
#include "utils/Validator.h"

namespace scapms::model {

FollowUpPatient::FollowUpPatient(std::string id, std::string name, int age, std::string contact,
                                 std::string previousDiagnosis, std::string lastVisitDate)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact)) {
    setPreviousDiagnosis(previousDiagnosis);
    setLastVisitDate(lastVisitDate);
}

void FollowUpPatient::setPreviousDiagnosis(const std::string& diagnosis) {
    utils::validate::requireText("Previous diagnosis", diagnosis);
    previousDiagnosis_ = utils::validate::trimmed(diagnosis);
}

void FollowUpPatient::setLastVisitDate(const std::string& date) {
    const std::string value = utils::validate::trimmed(date);
    utils::validate::date("Last visit date", value);
    if (value > utils::date::today()) {
        throw utils::ValidationException("Last visit date cannot be in the future.");
    }
    lastVisitDate_ = value;
}

std::string FollowUpPatient::details() const {
    return "Review of " + previousDiagnosis_ + " (last visit " + lastVisitDate_ + ")";
}

std::vector<std::string> FollowUpPatient::extraFields() const {
    return {previousDiagnosis_, lastVisitDate_};
}

std::unique_ptr<PatientRecord> FollowUpPatient::clone() const {
    return std::make_unique<FollowUpPatient>(*this);
}

void FollowUpPatient::scheduleAppointment(const Appointment& appointment) {
    if (appointment.date() <= lastVisitDate_) {
        throw utils::AppointmentException("A follow-up must be booked after the last visit (" +
                                          lastVisitDate_ + ").");
    }
    rules::requireRegularClinicHours(appointment, "Follow-up");
    attachAppointmentId(appointment.id());
}

void FollowUpPatient::cancelAppointment(const std::string& appointmentId) {
    if (!hasAppointment(appointmentId)) {
        throw utils::NotFoundException("Appointment " + appointmentId + " does not belong to " + id() + ".");
    }
    detachAppointmentId(appointmentId);
}

std::string FollowUpPatient::schedulingPolicy() const {
    return "Follow-up patient: after last visit (" + lastVisitDate_ + "), Monday-Friday, 08:00-17:00.";
}

} // namespace scapms::model
