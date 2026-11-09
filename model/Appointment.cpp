#include "model/Appointment.h"

#include "utils/Exceptions.h"
#include "utils/Validator.h"

#include <utility>

namespace scapms::model {

namespace v = utils::validate;

std::string statusToString(AppointmentStatus status) {
    switch (status) {
        case AppointmentStatus::Pending:   return "Pending";
        case AppointmentStatus::Completed: return "Completed";
        case AppointmentStatus::Cancelled: return "Cancelled";
    }
    return "Pending";
}

AppointmentStatus statusFromString(const std::string& text) {
    const std::string value = v::toLower(v::trimmed(text));
    if (value == "pending")   return AppointmentStatus::Pending;
    if (value == "completed") return AppointmentStatus::Completed;
    if (value == "cancelled") return AppointmentStatus::Cancelled;
    throw utils::ValidationException("Unknown appointment status: " + text);
}

Appointment::Appointment(std::string id, std::string patientId, std::string date, std::string time,
                         std::string service, std::string doctor, std::string notes,
                         AppointmentStatus status)
    : id_(v::trimmed(id)), patientId_(v::trimmed(patientId)), date_(v::trimmed(date)),
      time_(v::trimmed(time)), service_(v::trimmed(service)), doctor_(v::trimmed(doctor)),
      notes_(v::trimmed(notes)), status_(status) {
    v::requireText("Appointment ID", id_);
    v::requireText("Patient", patientId_);
    v::date("Appointment date", date_);
    v::time("Appointment time", time_);
    v::requireText("Medical service", service_);
    v::requireText("Doctor", doctor_);
    v::noDelimiter("Notes", notes_);
}

void Appointment::complete() {
    if (!isPending()) {
        throw utils::AppointmentException("Only pending appointments can be completed (" + id_ +
                                          " is " + statusName() + ").");
    }
    status_ = AppointmentStatus::Completed;
}

void Appointment::cancel() {
    if (!isPending()) {
        throw utils::AppointmentException("Only pending appointments can be cancelled (" + id_ +
                                          " is " + statusName() + ").");
    }
    status_ = AppointmentStatus::Cancelled;
}

void Appointment::restoreStatus(AppointmentStatus s) {
    status_ = s;
}

const std::vector<std::string>& Appointment::medicalServices() {
    static const std::vector<std::string> services = {
        "General Consultation", "Emergency Care",   "Follow-Up Review", "Laboratory Test",
        "Dental Check",         "Eye Clinic",       "Vaccination",      "Counselling"};
    return services;
}

const std::vector<std::string>& Appointment::doctors() {
    static const std::vector<std::string> doctors = {
        "Dr. A. Bello (General Practice)", "Dr. C. Okafor (Emergency)",
        "Dr. F. Musa (Dental)",            "Dr. T. Adeyemi (Laboratory)",
        "Dr. N. Eze (Ophthalmology)",      "Mrs. R. Ibrahim (Counsellor)"};
    return doctors;
}

} // namespace scapms::model
