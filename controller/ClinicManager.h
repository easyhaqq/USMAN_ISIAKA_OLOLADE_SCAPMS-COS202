#ifndef SCAPMS_CONTROLLER_CLINICMANAGER_H
#define SCAPMS_CONTROLLER_CLINICMANAGER_H

#include "controller/AppointmentController.h"
#include "controller/RecentPatientCache.h"
#include "controller/ReportGenerator.h"
#include "controller/SearchEngine.h"
#include "controller/SortEngine.h"
#include "controller/UndoManager.h"
#include "controller/WaitingQueue.h"
#include "model/ClinicDatabase.h"
#include "utils/IDGenerator.h"

#include <functional>
#include <string>
#include <vector>

namespace scapms::controller {

// One row of the waiting-queue display.
struct QueueRow {
    std::size_t position = 0;
    std::string patientId;
    std::string name;
    std::string category;
    std::string joinedAt;
    long long waitingSeconds = 0;
    int estimatedStartMinutes = 0;   // sum of consultation times of everyone ahead
};

struct SortRecommendation {
    SortAlgorithm algorithm;
    std::string reason;
};

// Facade between the GUI and the data. Owns the database and every data
// structure (vector, queue, stack, array cache). Contains no GUI code.
class ClinicManager {
public:
    ClinicManager();
    ClinicManager(const ClinicManager&) = delete;
    ClinicManager& operator=(const ClinicManager&) = delete;

    // ---- patients ----
    const std::vector<model::PatientPtr>& patients() const { return db_.patients(); }
    const model::ClinicDatabase& database() const { return db_; }
    std::string suggestedPatientId() const { return patientIds_.peek(); }
    bool isPatientIdTaken(const std::string& id) const { return db_.containsPatient(id); }

    const model::PatientRecord& registerPatient(model::PatientPtr patient);
    void updatePatient(model::PatientPtr updated);
    void deletePatient(const std::string& patientId);
    const model::PatientRecord* findPatient(const std::string& patientId);  // cache first, then linear search
    const model::PatientRecord* peekPatient(const std::string& patientId) const;  // lookup for display only

    // ---- appointments ----
    AppointmentController& appointments() { return appointments_; }
    const AppointmentController& appointments() const { return appointments_; }

    // ---- waiting queue ----
    void addToQueue(const std::string& patientId);
    QueueEntry callNextPatient();
    std::vector<QueueRow> queueRows() const;
    std::size_t queueSize() const { return queue_.size(); }

    // ---- search & sort ----
    SearchResult search(SearchField field, const std::string& keyword, SearchAlgorithm algorithm);
    SortStats sortPatients(model::SortKey key, SortAlgorithm algorithm);
    SortRecommendation recommendSort(model::SortKey key) const;
    model::SortKey sortedBy() const { return db_.sortedBy(); }

    // ---- undo ----
    bool canUndo() const { return undo_.canUndo(); }
    std::string undoLastOperation();
    std::vector<std::string> undoHistory() const { return undo_.history(); }

    // ---- cache ----
    const RecentPatientCache& recentCache() const { return cache_; }

    // ---- reports ----
    Report report(ReportType type, const std::string& date) const;

    // ---- persistence ----
    std::vector<std::string> load(const std::string& path);   // returns warnings
    void save(const std::string& path);
    void exportPatientsCsv(const std::string& path) const;
    void reset();
    bool hasUnsavedChanges() const { return modified_; }

private:
    using Comparator = std::function<bool(const model::PatientPtr&, const model::PatientPtr&)>;
    Comparator comparatorFor(model::SortKey key) const;
    std::size_t indexOrThrow(const std::string& patientId) const;
    void removePatientEverywhere(const std::string& patientId);
    void restorePatient(std::size_t index, const model::PatientRecord& snapshot,
                        const std::vector<model::Appointment>& appointments);
    void resetSessionState();
    void markModified() { modified_ = true; }

    model::ClinicDatabase db_;
    utils::IDGenerator patientIds_{"PAT-"};
    UndoManager undo_;                    // std::stack
    WaitingQueue queue_;                  // std::queue
    RecentPatientCache cache_;            // fixed-size array
    AppointmentController appointments_;
    bool modified_ = false;
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_CLINICMANAGER_H
