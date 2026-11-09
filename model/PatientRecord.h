#ifndef SCAPMS_MODEL_PATIENTRECORD_H
#define SCAPMS_MODEL_PATIENTRECORD_H

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

enum class PatientCategory { General, Emergency, FollowUp };

std::string categoryToString(PatientCategory category);       // "General", "Emergency", "Follow-Up"
PatientCategory categoryFromString(const std::string& text);  // throws ValidationException

// Abstract base class holding the information every patient has.
// It cannot be instantiated: category(), priorityLevel(), ... are pure virtual.
class PatientRecord {
public:
    PatientRecord(std::string id, std::string name, int age, std::string contact);
    virtual ~PatientRecord() = default;

    // ---- common data (encapsulated, validated setters) ----
    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    int age() const { return age_; }
    const std::string& contact() const { return contact_; }

    void setName(const std::string& name);
    void setAge(int age);
    void setContact(const std::string& contact);

    // ---- behaviour each patient type must define ----
    virtual PatientCategory category() const = 0;
    virtual int priorityLevel() const = 0;                 // 1 = most urgent
    virtual std::string priorityLabel() const = 0;         // "High Priority", "Normal", ...
    virtual int estimatedConsultationMinutes() const = 0;
    virtual std::string details() const = 0;               // type-specific summary
    virtual std::vector<std::string> extraFields() const = 0;  // for saving to file
    virtual std::unique_ptr<PatientRecord> clone() const = 0;  // deep copy for Undo

    // Non-virtual helpers built on the virtual functions above.
    std::string categoryName() const { return categoryToString(category()); }
    std::string summary() const;

    // ---- association with appointments (by appointment ID) ----
    const std::vector<std::string>& appointmentIds() const { return appointmentIds_; }
    void setAppointmentIds(std::vector<std::string> ids) { appointmentIds_ = std::move(ids); }
    bool hasAppointment(const std::string& appointmentId) const;
    // Low-level link/unlink with no rule checks (file loading and Undo).
    void attachAppointmentId(const std::string& appointmentId);
    void detachAppointmentId(const std::string& appointmentId);

protected:
    PatientRecord(const PatientRecord&) = default;           // only clone() may copy
    PatientRecord& operator=(const PatientRecord&) = default;

private:
    std::string id_;
    std::string name_;
    int age_ = 0;
    std::string contact_;
    std::vector<std::string> appointmentIds_;
};

using PatientPtr = std::unique_ptr<PatientRecord>;

} // namespace scapms::model

#endif // SCAPMS_MODEL_PATIENTRECORD_H
