#ifndef SCAPMS_UTILS_VALIDATOR_H
#define SCAPMS_UTILS_VALIDATOR_H

#include <string>

// Input validation shared by the model and the GUI. Every function throws a
// ValidationException (or InvalidAgeException) with a user-friendly message.
namespace scapms::utils::validate {

constexpr int kMinAge = 0;
constexpr int kMaxAge = 120;

std::string trimmed(const std::string& text);
std::string toLower(std::string text);
std::string toUpper(std::string text);

void requireText(const std::string& fieldName, const std::string& value);
void name(const std::string& value);
void age(int value);
int  parseAge(const std::string& text);           // "25" -> 25, throws on "abc" or "200"
void contact(const std::string& value);           // phone number or e-mail address
void patientId(const std::string& value);         // format PAT-0001
bool isPatientIdFormat(const std::string& value);
void date(const std::string& fieldName, const std::string& value);
void time(const std::string& fieldName, const std::string& value);
void noDelimiter(const std::string& fieldName, const std::string& value); // '|' is the file separator

} // namespace scapms::utils::validate

#endif // SCAPMS_UTILS_VALIDATOR_H
