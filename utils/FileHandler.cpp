#include "utils/FileHandler.h"

#include "model/PatientFactory.h"
#include "utils/Exceptions.h"
#include "utils/Validator.h"

#include <fstream>

namespace scapms::utils {

namespace {

enum class Section { None, Patients, Appointments };

std::ofstream openForWriting(const std::string& path) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) throw FileException("Could not open '" + path + "' for writing. Check the folder exists and is not read-only.");
    return out;
}

void finishWriting(std::ofstream& out, const std::string& path) {
    out.flush();
    if (!out) throw FileException("An error occurred while writing '" + path + "'.");
}

} // namespace

std::vector<std::string> FileHandler::split(const std::string& line, char delimiter) {
    std::vector<std::string> fields;
    std::string current;
    for (char c : line) {
        if (c == delimiter) {
            fields.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    fields.push_back(current);
    return fields;
}

void FileHandler::save(const std::string& path, const model::ClinicDatabase& database) {
    std::ofstream out = openForWriting(path);
    out << kHeader << '\n';

    out << "[PATIENTS]\n";
    for (const auto& patient : database.patients()) {
        out << patient->categoryName() << '|' << patient->id() << '|' << patient->name() << '|'
            << patient->age() << '|' << patient->contact();
        for (const auto& extra : patient->extraFields()) out << '|' << extra;
        out << '\n';
    }

    out << "[APPOINTMENTS]\n";
    for (const auto& a : database.appointments()) {
        out << a.id() << '|' << a.patientId() << '|' << a.date() << '|' << a.time() << '|'
            << a.service() << '|' << a.doctor() << '|' << a.statusName() << '|' << a.notes() << '\n';
    }
    finishWriting(out, path);
}

LoadResult FileHandler::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw FileException("Could not open '" + path + "'. The file may have been moved or deleted.");

    std::string line;
    if (!std::getline(in, line) || validate::trimmed(line) != kHeader) {
        throw FileException("'" + path + "' is not a SCAPMS data file (missing header line).");
    }

    LoadResult result;
    Section section = Section::None;
    int lineNumber = 1;

    while (std::getline(in, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();   // files edited on Windows
        if (validate::trimmed(line).empty() || line[0] == '#') continue;
        if (line == "[PATIENTS]")     { section = Section::Patients; continue; }
        if (line == "[APPOINTMENTS]") { section = Section::Appointments; continue; }

        try {
            const auto f = split(line, '|');
            if (section == Section::Patients) {
                if (f.size() < 6) throw ValidationException("expected at least 6 fields");
                model::PatientData data;
                data.category = model::categoryFromString(f[0]);
                data.id = f[1];
                data.name = f[2];
                data.age = f[3];
                data.contact = f[4];
                data.extras.assign(f.begin() + 5, f.end());
                result.database.addPatient(model::createPatient(data));
            } else if (section == Section::Appointments) {
                if (f.size() < 8) throw ValidationException("expected 8 fields");
                model::Appointment appointment(f[0], f[1], f[2], f[3], f[4], f[5], f[7],
                                               model::statusFromString(f[6]));
                const int owner = result.database.indexOfPatient(appointment.patientId());
                if (owner < 0) throw NotFoundException("patient " + appointment.patientId() + " does not exist");
                result.database.addAppointment(appointment);
                if (appointment.status() != model::AppointmentStatus::Cancelled) {
                    result.database.patients()[static_cast<std::size_t>(owner)]->attachAppointmentId(appointment.id());
                }
            } else {
                throw ValidationException("data found outside a [PATIENTS] or [APPOINTMENTS] section");
            }
        } catch (const ClinicException& error) {
            result.warnings.push_back("Line " + std::to_string(lineNumber) + " skipped: " + error.what());
        }
    }
    result.database.setSortedBy(model::SortKey::None);
    return result;
}

std::string FileHandler::csvField(const std::string& value) {
    if (value.find_first_of(",\"") == std::string::npos) return value;
    std::string quoted = "\"";
    for (char c : value) {
        if (c == '"') quoted += '"';   // escape quotes by doubling them
        quoted += c;
    }
    return quoted + "\"";
}

void FileHandler::exportPatientsCsv(const std::string& path, const model::ClinicDatabase& database) {
    std::ofstream out = openForWriting(path);
    out << "Patient ID,Name,Age,Contact,Category,Priority,Details,Appointments\n";
    for (const auto& p : database.patients()) {
        out << csvField(p->id()) << ',' << csvField(p->name()) << ',' << p->age() << ','
            << csvField(p->contact()) << ',' << csvField(p->categoryName()) << ','
            << csvField(p->priorityLabel()) << ',' << csvField(p->details()) << ','
            << p->appointmentIds().size() << '\n';
    }
    finishWriting(out, path);
}

void FileHandler::writeText(const std::string& path, const std::string& content) {
    std::ofstream out = openForWriting(path);
    out << content;
    finishWriting(out, path);
}

} // namespace scapms::utils
