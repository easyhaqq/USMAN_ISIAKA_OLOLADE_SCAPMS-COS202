#ifndef SCAPMS_MODEL_APPOINTMENT_H
#define SCAPMS_MODEL_APPOINTMENT_H

#include <string>
#include <vector>

namespace scapms::model {

enum class AppointmentStatus { Pending, Completed, Cancelled };

std::string statusToString(AppointmentStatus status);
AppointmentStatus statusFromString(const std::string& text);   // throws ValidationException

// One booking of a medical service for one patient. The patient is referenced
// by ID (association); ClinicDatabase owns all Appointment objects.
class Appointment {
public:
    Appointment(std::string id, std::string patientId, std::string date, std::string time,
                std::string service, std::string doctor, std::string notes = "",
                AppointmentStatus status = AppointmentStatus::Pending);

    const std::string& id() const { return id_; }
    const std::string& patientId() const { return patientId_; }
    const std::string& date() const { return date_; }
    const std::string& time() const { return time_; }
    const std::string& service() const { return service_; }
    const std::string& doctor() const { return doctor_; }
    const std::string& notes() const { return notes_; }
    AppointmentStatus status() const { return status_; }
    std::string statusName() const { return statusToString(status_); }
    std::string dateTime() const { return date_ + " " + time_; }
    bool isPending() const { return status_ == AppointmentStatus::Pending; }

    void complete();                         // Pending -> Completed
    void cancel();                           // Pending -> Cancelled
    void restoreStatus(AppointmentStatus s); // used only by Undo

    static const std::vector<std::string>& medicalServices();
    static const std::vector<std::string>& doctors();

private:
    std::string id_;
    std::string patientId_;
    std::string date_;
    std::string time_;
    std::string service_;
    std::string doctor_;
    std::string notes_;
    AppointmentStatus status_;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_APPOINTMENT_H
