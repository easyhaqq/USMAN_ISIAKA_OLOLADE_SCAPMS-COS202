#include "model/GeneralPatient.h"

#include "model/Appointment.h"
#include "model/SchedulingRules.h"
#include "utils/Exceptions.h"
#include "utils/Validator.h"

#include <algorithm>

namespace scapms::model {

const std::vector<std::string>& GeneralPatient::memberships() {
    static const std::vector<std::string> values = {"Student", "Staff", "Visitor"};
    return values;
}

GeneralPatient::GeneralPatient(std::string id, std::string name, int age, std::string contact,
                               std::string membership)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact)) {
    setMembership(membership);
}

void GeneralPatient::setMembership(const std::string& membership) {
    const std::string value = utils::validate::trimmed(membership);
    const auto& allowed = memberships();
    if (std::find(allowed.begin(), allowed.end(), value) == allowed.end()) {
        throw utils::ValidationException("Membership must be Student, Staff or Visitor.");
    }
    membership_ = value;
}

std::string GeneralPatient::details() const {
    return "Membership: " + membership_;
}

std::unique_ptr<PatientRecord> GeneralPatient::clone() const {
    return std::make_unique<GeneralPatient>(*this);
}

void GeneralPatient::scheduleAppointment(const Appointment& appointment) {
    rules::requireRegularClinicHours(appointment, "General");
    attachAppointmentId(appointment.id());
}

void GeneralPatient::cancelAppointment(const std::string& appointmentId) {
    if (!hasAppointment(appointmentId)) {
        throw utils::NotFoundException("Appointment " + appointmentId + " does not belong to " + id() + ".");
    }
    detachAppointmentId(appointmentId);
}

std::string GeneralPatient::schedulingPolicy() const {
    return "General patient: Monday-Friday, 08:00-17:00.";
}

} // namespace scapms::model
