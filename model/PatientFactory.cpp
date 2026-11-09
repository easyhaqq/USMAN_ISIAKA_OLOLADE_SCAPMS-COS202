#include "model/PatientFactory.h"

#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/Exceptions.h"
#include "utils/Validator.h"

namespace scapms::model {

namespace {

void requireExtras(const PatientData& data, std::size_t count) {
    if (data.extras.size() < count) {
        throw utils::ValidationException(categoryToString(data.category) + " patients need " +
                                         std::to_string(count) + " additional field(s).");
    }
}

int parseSeverity(const std::string& text) {
    const std::string value = utils::validate::trimmed(text);
    if (value.size() != 1 || value[0] < '0' || value[0] > '9') {
        throw utils::ValidationException("Emergency severity must be a number from 1 to 5.");
    }
    return value[0] - '0';
}

} // namespace

PatientPtr createPatient(const PatientData& data) {
    const int age = utils::validate::parseAge(data.age);

    switch (data.category) {
        case PatientCategory::General:
            requireExtras(data, 1);
            return std::make_unique<GeneralPatient>(data.id, data.name, age, data.contact,
                                                    data.extras[0]);
        case PatientCategory::Emergency:
            requireExtras(data, 2);
            return std::make_unique<EmergencyPatient>(data.id, data.name, age, data.contact,
                                                      parseSeverity(data.extras[0]), data.extras[1]);
        case PatientCategory::FollowUp:
            requireExtras(data, 2);
            return std::make_unique<FollowUpPatient>(data.id, data.name, age, data.contact,
                                                     data.extras[0], data.extras[1]);
    }
    throw utils::ValidationException("Unknown patient category.");
}

} // namespace scapms::model
