#include "controller/RecentPatientCache.h"

#include "model/PatientRecord.h"

namespace scapms::controller {

int RecentPatientCache::slotOf(const model::PatientRecord* patient) const {
    for (std::size_t i = 0; i < count_; ++i) {
        if (recentPatients_[i] == patient) return static_cast<int>(i);
    }
    return -1;
}

void RecentPatientCache::moveToFront(std::size_t slot) {
    const model::PatientRecord* patient = recentPatients_[slot];
    for (std::size_t i = slot; i > 0; --i) recentPatients_[i] = recentPatients_[i - 1];
    recentPatients_[0] = patient;
}

void RecentPatientCache::touch(const model::PatientRecord* patient) {
    if (!patient) return;
    const int slot = slotOf(patient);
    if (slot >= 0) {
        moveToFront(static_cast<std::size_t>(slot));
        return;
    }
    if (count_ < kCapacity) ++count_;               // otherwise the last slot is overwritten
    for (std::size_t i = count_ - 1; i > 0; --i) recentPatients_[i] = recentPatients_[i - 1];
    recentPatients_[0] = patient;
}

const model::PatientRecord* RecentPatientCache::find(const std::string& patientId) {
    for (std::size_t i = 0; i < count_; ++i) {
        if (recentPatients_[i]->id() == patientId) {
            ++hits_;
            moveToFront(i);
            return recentPatients_[0];
        }
    }
    ++misses_;
    return nullptr;
}

void RecentPatientCache::evict(const model::PatientRecord* patient) {
    const int slot = slotOf(patient);
    if (slot < 0) return;
    for (std::size_t i = static_cast<std::size_t>(slot); i + 1 < count_; ++i) {
        recentPatients_[i] = recentPatients_[i + 1];
    }
    recentPatients_[--count_] = nullptr;
}

void RecentPatientCache::replace(const model::PatientRecord* oldRecord, const model::PatientRecord* newRecord) {
    const int slot = slotOf(oldRecord);
    if (slot >= 0) recentPatients_[slot] = newRecord;
}

void RecentPatientCache::clear() {
    for (auto& slot : recentPatients_) slot = nullptr;
    count_ = hits_ = misses_ = 0;
}

const model::PatientRecord* RecentPatientCache::at(std::size_t slot) const {
    return slot < count_ ? recentPatients_[slot] : nullptr;
}

} // namespace scapms::controller
