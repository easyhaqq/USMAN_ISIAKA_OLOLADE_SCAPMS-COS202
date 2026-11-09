#ifndef SCAPMS_CONTROLLER_RECENTPATIENTCACHE_H
#define SCAPMS_CONTROLLER_RECENTPATIENTCACHE_H

#include <cstddef>
#include <string>

namespace scapms::model { class PatientRecord; }

namespace scapms::controller {

// "Frequently Accessed Patient Records" cache built on a fixed-size C++ array.
// Slot 0 is the most recently used record; when the array is full the least
// recently used record (last slot) is dropped. The cache does not own records.
class RecentPatientCache {
public:
    static constexpr std::size_t kCapacity = 10;

    void touch(const model::PatientRecord* patient);              // mark as just accessed
    const model::PatientRecord* find(const std::string& patientId); // counts a hit or a miss
    void evict(const model::PatientRecord* patient);        // record deleted
    void replace(const model::PatientRecord* oldRecord, const model::PatientRecord* newRecord);
    void clear();

    std::size_t size() const { return count_; }
    const model::PatientRecord* at(std::size_t slot) const;
    std::size_t hits() const { return hits_; }
    std::size_t misses() const { return misses_; }

private:
    int slotOf(const model::PatientRecord* patient) const;
    void moveToFront(std::size_t slot);

    const model::PatientRecord* recentPatients_[kCapacity] = {};
    std::size_t count_ = 0;
    std::size_t hits_ = 0;
    std::size_t misses_ = 0;
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_RECENTPATIENTCACHE_H
