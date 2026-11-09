#ifndef SCAPMS_UTILS_EXCEPTIONS_H
#define SCAPMS_UTILS_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace scapms::utils {

// Base class for every error the clinic system raises deliberately.
// The GUI catches this type and shows title() + what() in a dialog.
class ClinicException : public std::runtime_error {
public:
    explicit ClinicException(const std::string& message) : std::runtime_error(message) {}
    virtual std::string title() const { return "Clinic Error"; }
};

// Invalid or missing patient/appointment information.
class ValidationException : public ClinicException {
public:
    using ClinicException::ClinicException;
    std::string title() const override { return "Invalid Information"; }
};

// Age outside the accepted 0-120 range.
class InvalidAgeException : public ValidationException {
public:
    explicit InvalidAgeException(int age)
        : ValidationException("Patient age must be between 0 and 120 (entered: " +
                              std::to_string(age) + ").") {}
    std::string title() const override { return "Invalid Age"; }
};

// A patient ID that is already registered.
class DuplicateIdException : public ClinicException {
public:
    explicit DuplicateIdException(const std::string& id)
        : ClinicException("Patient ID " + id + " already exists.") {}
    std::string title() const override { return "Duplicate Patient ID"; }
};

// A patient, appointment or queue entry that could not be found.
class NotFoundException : public ClinicException {
public:
    using ClinicException::ClinicException;
    std::string title() const override { return "Record Not Found"; }
};

// Scheduling rules, clashes and invalid appointment state changes.
class AppointmentException : public ClinicException {
public:
    using ClinicException::ClinicException;
    std::string title() const override { return "Invalid Appointment"; }
};

// Files that cannot be opened, read or written.
class FileException : public ClinicException {
public:
    using ClinicException::ClinicException;
    std::string title() const override { return "File Error"; }
};

// Empty keywords, bad ID formats, or an algorithm that does not suit the data.
class SearchException : public ClinicException {
public:
    using ClinicException::ClinicException;
    std::string title() const override { return "Invalid Search"; }
};

} // namespace scapms::utils

#endif // SCAPMS_UTILS_EXCEPTIONS_H
