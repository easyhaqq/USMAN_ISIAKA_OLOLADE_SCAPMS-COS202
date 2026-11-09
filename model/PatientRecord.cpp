#include "model/PatientRecord.h"

#include "utils/Exceptions.h"
#include "utils/Validator.h"

#include <algorithm>

namespace scapms::model {

namespace v = utils::validate;

std::string categoryToString(PatientCategory category) {
    switch (category) {
        case PatientCategory::General:   return "General";
        case PatientCategory::Emergency: return "Emergency";
        case PatientCategory::FollowUp:  return "Follow-Up";
    }
    return "General";
}

PatientCategory categoryFromString(const std::string& text) {
    const std::string value = v::toLower(v::trimmed(text));
    if (value == "general")                         return PatientCategory::General;
    if (value == "emergency")                       return PatientCategory::Emergency;
    if (value == "follow-up" || value == "followup") return PatientCategory::FollowUp;
    throw utils::ValidationException("Unknown patient category: " + text);
}

PatientRecord::PatientRecord(std::string id, std::string name, int age, std::string contact) {
    id = v::toUpper(v::trimmed(id));
    v::patientId(id);
    id_ = id;
    setName(name);
    setAge(age);
    setContact(contact);
}

void PatientRecord::setName(const std::string& name) {
    v::name(name);
    name_ = v::trimmed(name);
}

void PatientRecord::setAge(int age) {
    v::age(age);
    age_ = age;
}

void PatientRecord::setContact(const std::string& contact) {
    v::contact(contact);
    contact_ = v::trimmed(contact);
}

std::string PatientRecord::summary() const {
    return id_ + " | " + name_ + " (" + std::to_string(age_) + ") | " + categoryName() +
           " | " + priorityLabel() + " | " + details();
}

bool PatientRecord::hasAppointment(const std::string& appointmentId) const {
    return std::find(appointmentIds_.begin(), appointmentIds_.end(), appointmentId) !=
           appointmentIds_.end();
}

void PatientRecord::attachAppointmentId(const std::string& appointmentId) {
    if (!hasAppointment(appointmentId)) appointmentIds_.push_back(appointmentId);
}

void PatientRecord::detachAppointmentId(const std::string& appointmentId) {
    appointmentIds_.erase(std::remove(appointmentIds_.begin(), appointmentIds_.end(), appointmentId),
                          appointmentIds_.end());
}

} // namespace scapms::model
