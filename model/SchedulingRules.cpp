#include "model/SchedulingRules.h"

#include "model/Appointment.h"
#include "utils/DateUtils.h"
#include "utils/Exceptions.h"

namespace scapms::model::rules {

void requireRegularClinicHours(const Appointment& appointment, const std::string& patientType) {
    if (utils::date::isWeekend(appointment.date())) {
        throw utils::AppointmentException(
            patientType + " patients can only be booked Monday to Friday (" + appointment.date() +
            " is a " + utils::date::dayName(appointment.date()) + ").");
    }
    if (appointment.time() < kOpeningTime || appointment.time() > kClosingTime) {
        throw utils::AppointmentException(patientType + " appointments must be between " +
                                          kOpeningTime + " and " + kClosingTime + ".");
    }
}

} // namespace scapms::model::rules
