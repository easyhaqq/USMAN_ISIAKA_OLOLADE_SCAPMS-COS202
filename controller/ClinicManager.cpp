#include "controller/ClinicManager.h"

#include "controller/PatientProcessor.h"
#include "utils/Exceptions.h"
#include "utils/FileHandler.h"
#include "utils/Validator.h"

#include <memory>
#include <unordered_map>

namespace scapms::controller {

using model::PatientPtr;
using model::PatientRecord;
using model::SortKey;

ClinicManager::ClinicManager()
    : appointments_(db_, undo_, [this] { markModified(); }) {}

// ================================================================= patients

std::size_t ClinicManager::indexOrThrow(const std::string& patientId) const {
    const int index = db_.indexOfPatient(patientId);
    if (index < 0) throw utils::NotFoundException("Patient " + patientId + " was not found.");
    return static_cast<std::size_t>(index);
}

const PatientRecord& ClinicManager::registerPatient(PatientPtr patient) {
    const PatientRecord& added = db_.addPatient(std::move(patient));   // throws on duplicate ID
    const std::string id = added.id();
    patientIds_.observe(id);
    cache_.touch(&added);
    undo_.record("Register patient " + id + " (" + added.name() + ")",
                 [this, id] { removePatientEverywhere(id); });
    markModified();
    return added;
}

void ClinicManager::updatePatient(PatientPtr updated) {
    if (!updated) throw utils::ValidationException("No patient information supplied.");
    const std::size_t index = indexOrThrow(updated->id());
    updated->setAppointmentIds(db_.patients()[index]->appointmentIds());   // keep the association

    PatientPtr old = db_.replacePatientAt(index, std::move(updated));
    cache_.replace(old.get(), db_.patients()[index].get());
    cache_.touch(db_.patients()[index].get());   // editing counts as an access

    const std::string id = old->id();
    std::shared_ptr<const PatientRecord> snapshot = std::move(old);
    undo_.record("Modify patient " + id + " (" + snapshot->name() + ")", [this, id, snapshot] {
        const std::size_t at = indexOrThrow(id);
        PatientPtr restored = snapshot->clone();
        restored->setAppointmentIds(db_.patients()[at]->appointmentIds());
        PatientPtr replaced = db_.replacePatientAt(at, std::move(restored));
        cache_.replace(replaced.get(), db_.patients()[at].get());
    });
    markModified();
}

void ClinicManager::removePatientEverywhere(const std::string& patientId) {
    const std::size_t index = indexOrThrow(patientId);
    // Collect IDs first: erasing from the vector would invalidate the pointers.
    std::vector<std::string> appointmentIds;
    for (const auto* appointment : db_.appointmentsFor(patientId)) appointmentIds.push_back(appointment->id());
    for (const auto& appointmentId : appointmentIds) db_.removeAppointment(appointmentId);
    queue_.remove(patientId);
    PatientPtr removed = db_.removePatientAt(index);
    cache_.evict(removed.get());
}

void ClinicManager::deletePatient(const std::string& patientId) {
    const std::size_t index = indexOrThrow(patientId);
    std::shared_ptr<const PatientRecord> snapshot = db_.patients()[index]->clone();
    std::vector<model::Appointment> appointments;
    for (const auto* appointment : db_.appointmentsFor(patientId)) appointments.push_back(*appointment);

    removePatientEverywhere(patientId);

    undo_.record("Delete patient " + patientId + " (" + snapshot->name() + ")",
                 [this, index, snapshot, appointments] { restorePatient(index, *snapshot, appointments); });
    markModified();
}

void ClinicManager::restorePatient(std::size_t index, const PatientRecord& snapshot,
                                   const std::vector<model::Appointment>& appointments) {
    db_.insertPatientAt(index, snapshot.clone());   // clone() also restores appointment links
    for (const auto& appointment : appointments) db_.addAppointment(appointment);
}

const PatientRecord* ClinicManager::findPatient(const std::string& patientId) {
    if (const PatientRecord* cached = cache_.find(patientId)) return cached;

    std::size_t comparisons = 0;
    const int index = SearchEngine::linearSearchById(db_.patients(), patientId, comparisons);
    if (index < 0) return nullptr;
    const PatientRecord* patient = db_.patients()[static_cast<std::size_t>(index)].get();
    cache_.touch(patient);
    return patient;
}

const PatientRecord* ClinicManager::peekPatient(const std::string& patientId) const {
    const int index = db_.indexOfPatient(patientId);
    return index < 0 ? nullptr : db_.patients()[static_cast<std::size_t>(index)].get();
}

// ============================================================ waiting queue

void ClinicManager::addToQueue(const std::string& patientId) {
    if (!findPatient(patientId)) throw utils::NotFoundException("Patient " + patientId + " was not found.");
    queue_.enqueue(patientId);
}

QueueEntry ClinicManager::callNextPatient() {
    return queue_.callNext();
}

std::vector<QueueRow> ClinicManager::queueRows() const {
    std::vector<QueueRow> rows;
    int minutesAhead = 0;
    std::size_t position = 1;
    for (const auto& entry : queue_.snapshot()) {
        const int index = db_.indexOfPatient(entry.patientId);
        if (index < 0) continue;
        const TriageInfo info = processPatient(*db_.patients()[static_cast<std::size_t>(index)]);
        rows.push_back({position++, info.id, info.name, info.category, entry.joinedTime(),
                        entry.waitingSeconds(), minutesAhead});
        minutesAhead += info.consultationMinutes;   // polymorphic consultation length
    }
    return rows;
}

// =========================================================== search & sort

SearchResult ClinicManager::search(SearchField field, const std::string& keyword, SearchAlgorithm algorithm) {
    SearchResult result = SearchEngine::search(db_, field, keyword, algorithm);
    if (!result.matches.empty()) cache_.touch(result.matches.front());
    return result;
}

ClinicManager::Comparator ClinicManager::comparatorFor(SortKey key) const {
    // Text keys are computed once before sorting instead of on every comparison.
    auto keys = std::make_shared<std::unordered_map<const PatientRecord*, std::string>>();
    switch (key) {
        case SortKey::Id:
            return [](const PatientPtr& a, const PatientPtr& b) { return a->id() < b->id(); };
        case SortKey::Age:
            return [](const PatientPtr& a, const PatientPtr& b) { return a->age() < b->age(); };
        case SortKey::Name:
            for (const auto& p : db_.patients()) (*keys)[p.get()] = utils::validate::toLower(p->name());
            return [keys](const PatientPtr& a, const PatientPtr& b) {
                return keys->at(a.get()) < keys->at(b.get());
            };
        case SortKey::AppointmentDate:
            for (const auto& p : db_.patients()) (*keys)[p.get()] = db_.nextAppointmentDate(p->id());
            return [keys](const PatientPtr& a, const PatientPtr& b) {
                const std::string& x = keys->at(a.get());
                const std::string& y = keys->at(b.get());
                if (x.empty()) return false;   // patients without a pending appointment go last
                if (y.empty()) return true;
                return x < y;
            };
        case SortKey::None:
            break;
    }
    throw utils::ValidationException("Choose a field to sort by.");
}

SortStats ClinicManager::sortPatients(SortKey key, SortAlgorithm algorithm) {
    SortStats stats = sortItems(db_.patients(), algorithm, comparatorFor(key));
    db_.setSortedBy(key);
    markModified();   // the saved order changes
    return stats;
}

SortRecommendation ClinicManager::recommendSort(SortKey key) const {
    const auto& list = db_.patients();
    const std::size_t n = list.size();
    std::size_t outOfOrder = 0;
    if (key != SortKey::None) {
        const Comparator less = comparatorFor(key);
        for (std::size_t i = 1; i < n; ++i) {
            if (less(list[i], list[i - 1])) ++outOfOrder;
        }
    }
    const std::string counts = std::to_string(n) + " records, " + std::to_string(outOfOrder) +
                               " adjacent pair(s) out of order";

    if (outOfOrder == 0) {
        return {SortAlgorithm::Insertion, "Already in order (" + counts + "): insertion sort confirms this in one pass."};
    }
    if (outOfOrder * 10 <= n) {
        return {SortAlgorithm::Insertion, "Nearly sorted (" + counts + "): insertion sort runs in close to O(n)."};
    }
    if (n <= 30) {
        return {SortAlgorithm::Insertion, "Small list (" + counts + "): insertion sort has the least overhead."};
    }
    if (key == SortKey::Age || key == SortKey::AppointmentDate) {
        return {SortAlgorithm::Merge, "Many equal keys (" + counts + "): stable O(n log n) merge sort keeps ties in order."};
    }
    return {SortAlgorithm::Quick, "Large unsorted list (" + counts + "): quick sort is fastest on average."};
}

// =================================================================== undo

std::string ClinicManager::undoLastOperation() {
    const std::string description = undo_.undoLast();
    markModified();
    return description;
}

// ================================================================ reports

Report ClinicManager::report(ReportType type, const std::string& date) const {
    return ReportGenerator(db_).generate(type, date);
}

// ============================================================ persistence

void ClinicManager::resetSessionState() {
    undo_.clear();
    queue_.clear();
    cache_.clear();
    patientIds_.reset();
    for (const auto& patient : db_.patients()) patientIds_.observe(patient->id());
    appointments_.syncIdsWithDatabase();
    modified_ = false;
}

std::vector<std::string> ClinicManager::load(const std::string& path) {
    utils::LoadResult result = utils::FileHandler::load(path);   // throws before touching current data
    db_ = std::move(result.database);
    resetSessionState();
    return result.warnings;
}

void ClinicManager::save(const std::string& path) {
    utils::FileHandler::save(path, db_);
    modified_ = false;
}

void ClinicManager::exportPatientsCsv(const std::string& path) const {
    utils::FileHandler::exportPatientsCsv(path, db_);
}

void ClinicManager::reset() {
    db_.clear();
    resetSessionState();
}

} // namespace scapms::controller
