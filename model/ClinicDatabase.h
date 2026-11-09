#ifndef SCAPMS_MODEL_CLINICDATABASE_H
#define SCAPMS_MODEL_CLINICDATABASE_H

#include "model/Appointment.h"
#include "model/PatientRecord.h"

#include <string>
#include <vector>

namespace scapms::model {

// Which field the patient vector is currently ordered by. Searching uses this
// to decide whether binary search is possible.
enum class SortKey { None, Id, Name, Age, AppointmentDate };
std::string sortKeyToString(SortKey key);

// Owns every patient (std::vector of unique_ptr -> composition) and every appointment.
// It enforces unique IDs and remembers the sort state of the patient vector.
class ClinicDatabase {
public:
    ClinicDatabase() = default;
    ClinicDatabase(const ClinicDatabase&) = delete;
    ClinicDatabase& operator=(const ClinicDatabase&) = delete;
    ClinicDatabase(ClinicDatabase&&) = default;
    ClinicDatabase& operator=(ClinicDatabase&&) = default;

    // ---- patients ----
    std::vector<PatientPtr>& patients() { return patients_; }
    const std::vector<PatientPtr>& patients() const { return patients_; }
    std::size_t patientCount() const { return patients_.size(); }

    PatientRecord& addPatient(PatientPtr patient);                    // throws DuplicateIdException
    void insertPatientAt(std::size_t index, PatientPtr patient);      // used by Undo
    PatientPtr removePatientAt(std::size_t index);
    PatientPtr replacePatientAt(std::size_t index, PatientPtr patient); // returns the old record
    int indexOfPatient(const std::string& patientId) const;          // -1 if absent
    bool containsPatient(const std::string& patientId) const { return indexOfPatient(patientId) >= 0; }

    // ---- appointments ----
    std::vector<Appointment>& appointments() { return appointments_; }
    const std::vector<Appointment>& appointments() const { return appointments_; }

    void addAppointment(const Appointment& appointment);  // throws on duplicate appointment ID
    Appointment removeAppointment(const std::string& appointmentId);
    Appointment* findAppointment(const std::string& appointmentId);
    const Appointment* findAppointment(const std::string& appointmentId) const;
    std::vector<const Appointment*> appointmentsFor(const std::string& patientId) const;
    std::string nextAppointmentDate(const std::string& patientId) const; // earliest pending, "" if none

    // ---- sort state ----
    SortKey sortedBy() const { return sortedBy_; }
    void setSortedBy(SortKey key) { sortedBy_ = key; }

    void clear();

private:
    std::vector<PatientPtr> patients_;
    std::vector<Appointment> appointments_;
    SortKey sortedBy_ = SortKey::None;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_CLINICDATABASE_H
