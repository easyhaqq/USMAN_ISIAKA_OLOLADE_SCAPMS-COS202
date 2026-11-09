#include "controller/WaitingQueue.h"

#include "utils/Exceptions.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace scapms::controller {

long long QueueEntry::waitingSeconds() const {
    const auto elapsed = std::chrono::system_clock::now() - joinedAt;
    return std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
}

std::string QueueEntry::joinedTime() const {
    const std::time_t t = std::chrono::system_clock::to_time_t(joinedAt);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%H:%M:%S");
    return out.str();
}

void WaitingQueue::enqueue(const std::string& patientId) {
    if (contains(patientId)) {
        throw utils::ClinicException("Patient " + patientId + " is already in the waiting queue.");
    }
    queue_.push(QueueEntry{patientId, std::chrono::system_clock::now()});
}

QueueEntry WaitingQueue::callNext() {
    if (queue_.empty()) throw utils::NotFoundException("The waiting queue is empty.");
    QueueEntry next = queue_.front();
    queue_.pop();
    return next;
}

void WaitingQueue::remove(const std::string& patientId) {
    std::queue<QueueEntry> kept;
    while (!queue_.empty()) {
        if (queue_.front().patientId != patientId) kept.push(queue_.front());
        queue_.pop();
    }
    queue_ = std::move(kept);   // relative order of everyone else is preserved
}

bool WaitingQueue::contains(const std::string& patientId) const {
    for (const auto& entry : snapshot()) {
        if (entry.patientId == patientId) return true;
    }
    return false;
}

const QueueEntry& WaitingQueue::front() const {
    if (queue_.empty()) throw utils::NotFoundException("The waiting queue is empty.");
    return queue_.front();
}

std::vector<QueueEntry> WaitingQueue::snapshot() const {
    std::vector<QueueEntry> entries;
    std::queue<QueueEntry> copy = queue_;   // a queue can only be read from the front
    while (!copy.empty()) {
        entries.push_back(copy.front());
        copy.pop();
    }
    return entries;
}

} // namespace scapms::controller
