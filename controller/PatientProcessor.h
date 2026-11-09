#ifndef SCAPMS_CONTROLLER_PATIENTPROCESSOR_H
#define SCAPMS_CONTROLLER_PATIENTPROCESSOR_H

#include "model/PatientRecord.h"

#include <string>

namespace scapms::controller {

// Result of triaging one patient.
struct TriageInfo {
    std::string id;
    std::string name;
    std::string category;
    int priorityLevel = 0;
    std::string priorityLabel;
    int consultationMinutes = 0;
    std::string details;
    std::string schedulingPolicy;
};

// Runtime polymorphism: works on ANY PatientRecord through a base-class
// reference. Each call below is dispatched to the real subclass at run time.
TriageInfo processPatient(const model::PatientRecord& patient);

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_PATIENTPROCESSOR_H
