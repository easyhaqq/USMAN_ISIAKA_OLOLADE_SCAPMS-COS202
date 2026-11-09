#include "controller/PatientProcessor.h"

#include "model/Appointable.h"

namespace scapms::controller {

TriageInfo processPatient(const model::PatientRecord& patient) {
    TriageInfo info;
    info.id = patient.id();
    info.name = patient.name();
    info.category = patient.categoryName();                       // virtual category()
    info.priorityLevel = patient.priorityLevel();                 // virtual
    info.priorityLabel = patient.priorityLabel();                 // virtual
    info.consultationMinutes = patient.estimatedConsultationMinutes();  // virtual
    info.details = patient.details();                             // virtual

    // Cross-cast to the interface: only types that implement Appointable answer.
    const auto* appointable = dynamic_cast<const model::Appointable*>(&patient);
    info.schedulingPolicy = appointable ? appointable->schedulingPolicy() : "Not schedulable";
    return info;
}

} // namespace scapms::controller
