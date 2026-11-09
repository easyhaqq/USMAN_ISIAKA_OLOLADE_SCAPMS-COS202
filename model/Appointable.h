#ifndef SCAPMS_MODEL_APPOINTABLE_H
#define SCAPMS_MODEL_APPOINTABLE_H

#include <string>

namespace scapms::model {

class Appointment;

// Interface (pure abstract class) for patient types that can be booked.
// Each implementation enforces the scheduling rules of its own patient type.
class Appointable {
public:
    virtual ~Appointable() = default;

    // Throws AppointmentException if the slot breaks this patient type's rules.
    virtual void scheduleAppointment(const Appointment& appointment) = 0;
    // Throws if the appointment is not linked or this type forbids cancelling it.
    virtual void cancelAppointment(const std::string& appointmentId) = 0;
    // Human-readable description of the rules, shown in the GUI.
    virtual std::string schedulingPolicy() const = 0;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_APPOINTABLE_H
