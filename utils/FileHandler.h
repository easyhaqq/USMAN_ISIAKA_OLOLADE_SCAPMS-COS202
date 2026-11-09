#ifndef SCAPMS_UTILS_FILEHANDLER_H
#define SCAPMS_UTILS_FILEHANDLER_H

#include "model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::utils {

// Outcome of loading a data file. Lines that fail validation are skipped and
// reported as warnings so one bad line does not lose the whole file.
struct LoadResult {
    model::ClinicDatabase database;
    std::vector<std::string> warnings;
};

// Saves and loads clinic data as a sectioned, pipe-delimited text file:
//
//   # SCAPMS data file v1
//   [PATIENTS]
//   category|id|name|age|contact|extra1|extra2
//   [APPOINTMENTS]
//   id|patientId|date|time|service|doctor|status|notes
//
// Every function throws FileException if the file cannot be opened or written.
class FileHandler {
public:
    static constexpr const char* kHeader = "# SCAPMS data file v1";

    static void save(const std::string& path, const model::ClinicDatabase& database);
    static LoadResult load(const std::string& path);
    static void exportPatientsCsv(const std::string& path, const model::ClinicDatabase& database);
    static void writeText(const std::string& path, const std::string& content);

    static std::vector<std::string> split(const std::string& line, char delimiter);

private:
    static std::string csvField(const std::string& value);
};

} // namespace scapms::utils

#endif // SCAPMS_UTILS_FILEHANDLER_H
