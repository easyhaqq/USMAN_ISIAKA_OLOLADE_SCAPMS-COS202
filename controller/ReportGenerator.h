#ifndef SCAPMS_CONTROLLER_REPORTGENERATOR_H
#define SCAPMS_CONTROLLER_REPORTGENERATOR_H

#include "model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::controller {

// A report as plain data, so the GUI can show it in a table and export it as text.
struct Report {
    std::string title;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> summary;

    std::string toText() const;   // aligned plain-text version for exporting
};

enum class ReportType { MostAttended, PendingAppointments, CategoryDistribution, DailySummary, TriageOverview };

const std::vector<ReportType>& allReportTypes();
std::string reportName(ReportType type);

class ReportGenerator {
public:
    explicit ReportGenerator(const model::ClinicDatabase& database) : db_(database) {}

    Report generate(ReportType type, const std::string& date) const;

    Report mostFrequentlyAttended(std::size_t limit = 10) const;
    Report pendingAppointments() const;
    Report categoryDistribution() const;
    Report dailySummary(const std::string& date) const;
    Report triageOverview() const;   // every patient processed through a base-class reference

private:
    const model::ClinicDatabase& db_;
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_REPORTGENERATOR_H
