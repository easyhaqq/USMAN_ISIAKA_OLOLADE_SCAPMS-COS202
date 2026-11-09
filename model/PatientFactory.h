#ifndef SCAPMS_MODEL_PATIENTFACTORY_H
#define SCAPMS_MODEL_PATIENTFACTORY_H

#include "model/PatientRecord.h"

#include <string>
#include <vector>

namespace scapms::model {

// Raw, unvalidated patient data as entered in the GUI or read from a file.
struct PatientData {
    PatientCategory category = PatientCategory::General;
    std::string id;
    std::string name;
    std::string age;        // kept as text so the age is validated in one place
    std::string contact;
    std::vector<std::string> extras;   // type-specific fields, in extraFields() order
};

// Creates the correct subclass. Throws ValidationException on any invalid field.
PatientPtr createPatient(const PatientData& data);

} // namespace scapms::model

#endif // SCAPMS_MODEL_PATIENTFACTORY_H
