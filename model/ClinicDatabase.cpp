#include "model/ClinicDatabase.h"

#include "utils/Exceptions.h"

namespace scapms::model {

std::string sortKeyToString(SortKey key) {
    switch (key) {
        case SortKey::None:            return "Unsorted";
        case SortKey::Id:              return "Patient ID";
        case SortKey::Name:            return "Patient Name";
        case SortKey::Age:             return "Age";
        case SortKey::AppointmentDate: return "Next Appointment Date";
    }
    return "Unsorted";
}

PatientRecord& ClinicDatabase::addPatient(PatientPtr patient) {
    if (!patient) throw utils::ValidationException("Cannot add an empty patient record.");
    if (containsPatient(patient->id())) throw utils::DuplicateIdException(patient->id());
    patients_.push_back(std::move(patient));
    sortedBy_ = SortKey::None;   // appended at the end -> order no longer guaranteed
    return *patients_.back();
}

void ClinicDatabase::insertPatientAt(std::size_t index, PatientPtr patient) {
    if (!patient) throw utils::ValidationException("Cannot insert an empty patient record.");
    if (containsPatient(patient->id())) throw utils::DuplicateIdException(patient->id());
    if (index > patients_.size()) index = patients_.size();
    patients_.insert(patients_.begin() + static_cast<std::ptrdiff_t>(index), std::move(patient));
    sortedBy_ = SortKey::None;
}

PatientPtr ClinicDatabase::removePatientAt(std::size_t index) {
    if (index >= patients_.size()) throw utils::NotFoundException("Patient index out of range.");
    PatientPtr removed = std::move(patients_[index]);
    patients_.erase(patients_.begin() + static_cast<std::ptrdiff_t>(index));
    return removed;   // removing keeps the remaining order, so sortedBy_ stays valid
}

PatientPtr ClinicDatabase::replacePatientAt(std::size_t index, PatientPtr patient) {
    if (index >= patients_.size()) throw utils::NotFoundException("Patient index out of range.");
    if (!patient || patient->id() != patients_[index]->id()) {
        throw utils::ValidationException("A replacement record must keep the same patient ID.");
    }
    PatientPtr old = std::move(patients_[index]);
    patients_[index] = std::move(patient);
    if (sortedBy_ != SortKey::Id) sortedBy_ = SortKey::None;   // IDs never change on edit
    return old;
}

int ClinicDatabase::indexOfPatient(const std::string& patientId) const {
    for (std::size_t i = 0; i < patients_.size(); ++i) {
        if (patients_[i]->id() == patientId) return static_cast<int>(i);
    }
    return -1;
}

void ClinicDatabase::addAppointment(const Appointment& appointment) {
    if (findAppointment(appointment.id())) {
        throw utils::AppointmentException("Appointment ID " + appointment.id() + " already exists.");
    }
    appointments_.push_back(appointment);
    if (sortedBy_ == SortKey::AppointmentDate) sortedBy_ = SortKey::None;
}

Appointment ClinicDatabase::removeAppointment(const std::string& appointmentId) {
    for (auto it = appointments_.begin(); it != appointments_.end(); ++it) {
        if (it->id() == appointmentId) {
            Appointment removed = *it;
            appointments_.erase(it);
            if (sortedBy_ == SortKey::AppointmentDate) sortedBy_ = SortKey::None;
            return removed;
        }
    }
    throw utils::NotFoundException("Appointment " + appointmentId + " was not found.");
}

Appointment* ClinicDatabase::findAppointment(const std::string& appointmentId) {
    for (auto& appointment : appointments_) {
        if (appointment.id() == appointmentId) return &appointment;
    }
    return nullptr;
}

const Appointment* ClinicDatabase::findAppointment(const std::string& appointmentId) const {
    for (const auto& appointment : appointments_) {
        if (appointment.id() == appointmentId) return &appointment;
    }
    return nullptr;
}

std::vector<const Appointment*> ClinicDatabase::appointmentsFor(const std::string& patientId) const {
    std::vector<const Appointment*> result;
    for (const auto& appointment : appointments_) {
        if (appointment.patientId() == patientId) result.push_back(&appointment);
    }
    return result;
}

std::string ClinicDatabase::nextAppointmentDate(const std::string& patientId) const {
    std::string earliest;
    for (const auto& appointment : appointments_) {
        if (appointment.patientId() == patientId && appointment.isPending() &&
            (earliest.empty() || appointment.dateTime() < earliest)) {
            earliest = appointment.dateTime();
        }
    }
    return earliest;
}

void ClinicDatabase::clear() {
    patients_.clear();
    appointments_.clear();
    sortedBy_ = SortKey::None;
}

} // namespace scapms::model
