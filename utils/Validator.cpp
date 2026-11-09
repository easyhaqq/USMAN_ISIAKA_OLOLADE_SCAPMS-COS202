#include "utils/Validator.h"

#include "utils/DateUtils.h"
#include "utils/Exceptions.h"

#include <algorithm>
#include <cctype>
#include <regex>

namespace scapms::utils::validate {

std::string trimmed(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

std::string toUpper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return text;
}

void noDelimiter(const std::string& fieldName, const std::string& value) {
    if (value.find('|') != std::string::npos || value.find('\n') != std::string::npos) {
        throw ValidationException(fieldName + " must not contain the '|' character or line breaks.");
    }
}

void requireText(const std::string& fieldName, const std::string& value) {
    if (trimmed(value).empty()) throw ValidationException(fieldName + " is required.");
    noDelimiter(fieldName, value);
}

void name(const std::string& value) {
    requireText("Patient name", value);
    static const std::regex pattern(R"(^[A-Za-z][A-Za-z .'\-]{1,59}$)");
    if (!std::regex_match(trimmed(value), pattern)) {
        throw ValidationException(
            "Patient name must be 2-60 characters and contain only letters, spaces, "
            "apostrophes, hyphens or full stops.");
    }
}

void age(int value) {
    if (value < kMinAge || value > kMaxAge) throw InvalidAgeException(value);
}

int parseAge(const std::string& text) {
    const std::string value = trimmed(text);
    static const std::regex digitsOnly(R"(^-?\d{1,4}$)");
    if (!std::regex_match(value, digitsOnly)) {
        throw ValidationException("Patient age must be a whole number between 0 and 120.");
    }
    const int parsed = std::stoi(value);
    age(parsed);
    return parsed;
}

void contact(const std::string& value) {
    requireText("Contact information", value);
    static const std::regex phone(R"(^\+?[0-9][0-9 \-]{6,17}$)");
    static const std::regex email(R"(^[\w.+\-]+@[\w\-]+(\.[\w\-]+)+$)");
    const std::string v = trimmed(value);
    if (!std::regex_match(v, phone) && !std::regex_match(v, email)) {
        throw ValidationException(
            "Contact must be a phone number (e.g. 08031234567 or +2348031234567) "
            "or an e-mail address.");
    }
}

bool isPatientIdFormat(const std::string& value) {
    static const std::regex pattern(R"(^PAT-\d{4}$)");
    return std::regex_match(value, pattern);
}

void patientId(const std::string& value) {
    if (!isPatientIdFormat(value)) {
        throw ValidationException("Patient ID must have the format PAT-0001 (PAT- followed by 4 digits).");
    }
}

void date(const std::string& fieldName, const std::string& value) {
    if (!utils::date::isValidDate(value)) {
        throw ValidationException(fieldName + " must be a valid date in the format YYYY-MM-DD.");
    }
}

void time(const std::string& fieldName, const std::string& value) {
    if (!utils::date::isValidTime(value)) {
        throw ValidationException(fieldName + " must be a valid 24-hour time in the format HH:MM.");
    }
}

} // namespace scapms::utils::validate
