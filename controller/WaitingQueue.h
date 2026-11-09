#ifndef SCAPMS_CONTROLLER_WAITINGQUEUE_H
#define SCAPMS_CONTROLLER_WAITINGQUEUE_H

#include <chrono>
#include <queue>
#include <string>
#include <vector>

namespace scapms::controller {

struct QueueEntry {
    std::string patientId;
    std::chrono::system_clock::time_point joinedAt;

    long long waitingSeconds() const;
    std::string joinedTime() const;   // "HH:MM:SS"
};

// Patients waiting for consultation, served first-in first-out with std::queue.
class WaitingQueue {
public:
    void enqueue(const std::string& patientId);   // throws if already waiting
    QueueEntry callNext();                        // throws if the queue is empty
    void remove(const std::string& patientId);    // used when a patient is deleted
    bool contains(const std::string& patientId) const;
    const QueueEntry& front() const;
    std::vector<QueueEntry> snapshot() const;     // front first, for display
    std::size_t size() const { return queue_.size(); }
    bool empty() const { return queue_.empty(); }
    void clear() { queue_ = std::queue<QueueEntry>(); }

private:
    std::queue<QueueEntry> queue_;
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_WAITINGQUEUE_H
