#ifndef SCAPMS_MODEL_SCHEDULINGRULES_H
#define SCAPMS_MODEL_SCHEDULINGRULES_H

#include <string>

namespace scapms::model {

class Appointment;

// Rules shared by the patient types that use regular clinic hours.
namespace rules {

constexpr const char* kOpeningTime = "08:00";
constexpr const char* kClosingTime = "17:00";

// Throws AppointmentException unless the slot is Monday-Friday, 08:00-17:00.
void requireRegularClinicHours(const Appointment& appointment, const std::string& patientType);

} // namespace rules
} // namespace scapms::model

#endif // SCAPMS_MODEL_SCHEDULINGRULES_H
