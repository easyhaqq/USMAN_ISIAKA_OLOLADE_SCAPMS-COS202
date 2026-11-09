#ifndef SCAPMS_CONTROLLER_APPOINTMENTCONTROLLER_H
#define SCAPMS_CONTROLLER_APPOINTMENTCONTROLLER_H

#include "controller/UndoManager.h"
#include "model/ClinicDatabase.h"
#include "utils/IDGenerator.h"

#include <functional>
#include <string>
#include <vector>

namespace scapms::model { class Appointable; }

namespace scapms::controller {

// Business rules for appointments: past dates, double booking, patient-type
// policies (through the Appointable interface) and undo records.
class AppointmentController {
public:
    AppointmentController(model::ClinicDatabase& database, UndoManager& undo,
                          std::function<void()> onModified);

    std::string schedule(const std::string& patientId, const std::string& date,
                         const std::string& time, const std::string& service,
                         const std::string& doctor, const std::string& notes);
    void cancel(const std::string& appointmentId);
    void complete(const std::string& appointmentId);
    void remove(const std::string& appointmentId);

    std::string nextId() const { return ids_.peek(); }
    std::string policyFor(const std::string& patientId) const;

    // Pending appointments today that start within the next `windowMinutes`.
    std::vector<const model::Appointment*> dueSoon(int windowMinutes) const;
    std::vector<const model::Appointment*> forDate(const std::string& date) const;
    // Earliest pending appointment dated today or earlier (for "Call next patient").
    const model::Appointment* consultationFor(const std::string& patientId) const;

    void syncIdsWithDatabase();   // after loading a file or starting a new one

private:
    model::PatientRecord& patientOf(const std::string& patientId) const;
    model::Appointment& appointmentOf(const std::string& appointmentId) const;
    void checkClashes(const model::Appointment& candidate) const;
    void dateOrderChanged();

    model::ClinicDatabase& db_;
    UndoManager& undo_;
    std::function<void()> onModified_;
    utils::IDGenerator ids_{"APT-"};
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_APPOINTMENTCONTROLLER_H
