#include "controller/AppointmentController.h"

#include "model/Appointable.h"
#include "utils/DateUtils.h"
#include "utils/Exceptions.h"

namespace scapms::controller {

using model::Appointment;
using model::AppointmentStatus;

AppointmentController::AppointmentController(model::ClinicDatabase& database, UndoManager& undo,
                                             std::function<void()> onModified)
    : db_(database), undo_(undo), onModified_(std::move(onModified)) {}

model::PatientRecord& AppointmentController::patientOf(const std::string& patientId) const {
    const int index = db_.indexOfPatient(patientId);
    if (index < 0) throw utils::NotFoundException("Patient " + patientId + " was not found.");
    return *db_.patients()[static_cast<std::size_t>(index)];
}

model::Appointment& AppointmentController::appointmentOf(const std::string& appointmentId) const {
    Appointment* appointment = db_.findAppointment(appointmentId);
    if (!appointment) throw utils::NotFoundException("Appointment " + appointmentId + " was not found.");
    return *appointment;
}

void AppointmentController::dateOrderChanged() {
    if (db_.sortedBy() == model::SortKey::AppointmentDate) db_.setSortedBy(model::SortKey::None);
    if (onModified_) onModified_();
}

void AppointmentController::checkClashes(const Appointment& candidate) const {
    for (const auto& existing : db_.appointments()) {
        if (!existing.isPending() || existing.dateTime() != candidate.dateTime()) continue;
        if (existing.doctor() == candidate.doctor()) {
            throw utils::AppointmentException(candidate.doctor() + " is already booked at " +
                                              candidate.dateTime() + " (" + existing.id() + ").");
        }
        if (existing.patientId() == candidate.patientId()) {
            throw utils::AppointmentException("This patient already has an appointment at " +
                                              candidate.dateTime() + " (" + existing.id() + ").");
        }
    }
}

std::string AppointmentController::schedule(const std::string& patientId, const std::string& date,
                                            const std::string& time, const std::string& service,
                                            const std::string& doctor, const std::string& notes) {
    model::PatientRecord& patient = patientOf(patientId);
    auto* appointable = dynamic_cast<model::Appointable*>(&patient);
    if (!appointable) throw utils::AppointmentException(patient.categoryName() + " patients cannot be scheduled.");

    // The constructor validates the date/time format and required fields.
    Appointment appointment(ids_.peek(), patientId, date, time, service, doctor, notes);

    const std::string today = utils::date::today();
    if (appointment.date() < today) {
        throw utils::AppointmentException("Appointment date cannot be in the past.");
    }
    if (appointment.date() == today && appointment.time() < utils::date::currentTime()) {
        throw utils::AppointmentException("That time has already passed today. Choose a later time.");
    }
    checkClashes(appointment);
    appointable->scheduleAppointment(appointment);   // patient-type rules (polymorphic)

    ids_.next();
    db_.addAppointment(appointment);
    const std::string id = appointment.id();
    undo_.record("Schedule appointment " + id + " for " + patient.name(), [this, id, patientId] {
        db_.removeAppointment(id);
        const int index = db_.indexOfPatient(patientId);
        if (index >= 0) db_.patients()[static_cast<std::size_t>(index)]->detachAppointmentId(id);
    });
    dateOrderChanged();
    return id;
}

void AppointmentController::cancel(const std::string& appointmentId) {
    Appointment& appointment = appointmentOf(appointmentId);
    if (!appointment.isPending()) {
        throw utils::AppointmentException("Only pending appointments can be cancelled (" + appointmentId +
                                          " is " + appointment.statusName() + ").");
    }
    model::PatientRecord& patient = patientOf(appointment.patientId());
    if (auto* appointable = dynamic_cast<model::Appointable*>(&patient)) {
        appointable->cancelAppointment(appointmentId);   // may refuse, e.g. critical emergency
    }
    appointment.cancel();

    const std::string patientId = patient.id();
    undo_.record("Cancel appointment " + appointmentId + " (" + patient.name() + ")",
                 [this, appointmentId, patientId] {
                     appointmentOf(appointmentId).restoreStatus(AppointmentStatus::Pending);
                     const int index = db_.indexOfPatient(patientId);
                     if (index >= 0) db_.patients()[static_cast<std::size_t>(index)]->attachAppointmentId(appointmentId);
                 });
    dateOrderChanged();
}

void AppointmentController::complete(const std::string& appointmentId) {
    Appointment& appointment = appointmentOf(appointmentId);
    if (appointment.date() > utils::date::today()) {
        throw utils::AppointmentException("Appointment " + appointmentId + " is on " + appointment.date() +
                                          " and cannot be completed before that day.");
    }
    appointment.complete();
    undo_.record("Complete appointment " + appointmentId, [this, appointmentId] {
        appointmentOf(appointmentId).restoreStatus(AppointmentStatus::Pending);
    });
    dateOrderChanged();
}

void AppointmentController::remove(const std::string& appointmentId) {
    Appointment removed = db_.removeAppointment(appointmentId);
    const int index = db_.indexOfPatient(removed.patientId());
    if (index >= 0) db_.patients()[static_cast<std::size_t>(index)]->detachAppointmentId(appointmentId);

    undo_.record("Remove appointment record " + appointmentId, [this, removed] {
        db_.addAppointment(removed);
        const int owner = db_.indexOfPatient(removed.patientId());
        if (owner >= 0 && removed.status() != AppointmentStatus::Cancelled) {
            db_.patients()[static_cast<std::size_t>(owner)]->attachAppointmentId(removed.id());
        }
    });
    dateOrderChanged();
}

std::string AppointmentController::policyFor(const std::string& patientId) const {
    const auto* appointable = dynamic_cast<const model::Appointable*>(&patientOf(patientId));
    return appointable ? appointable->schedulingPolicy() : "This patient type cannot be scheduled.";
}

std::vector<const Appointment*> AppointmentController::dueSoon(int windowMinutes) const {
    std::vector<const Appointment*> due;
    const std::string today = utils::date::today();
    const int now = utils::date::toMinutes(utils::date::currentTime());
    for (const auto& appointment : db_.appointments()) {
        if (!appointment.isPending() || appointment.date() != today) continue;
        const int start = utils::date::toMinutes(appointment.time());
        if (start >= now && start - now <= windowMinutes) due.push_back(&appointment);
    }
    return due;
}

std::vector<const Appointment*> AppointmentController::forDate(const std::string& date) const {
    std::vector<const Appointment*> result;
    for (const auto& appointment : db_.appointments()) {
        if (appointment.date() == date) result.push_back(&appointment);
    }
    return result;
}

const Appointment* AppointmentController::consultationFor(const std::string& patientId) const {
    const Appointment* earliest = nullptr;
    const std::string today = utils::date::today();
    for (const auto& appointment : db_.appointments()) {
        if (appointment.patientId() == patientId && appointment.isPending() && appointment.date() <= today &&
            (!earliest || appointment.dateTime() < earliest->dateTime())) {
            earliest = &appointment;
        }
    }
    return earliest;
}

void AppointmentController::syncIdsWithDatabase() {
    ids_.reset();
    for (const auto& appointment : db_.appointments()) ids_.observe(appointment.id());
}

} // namespace scapms::controller
