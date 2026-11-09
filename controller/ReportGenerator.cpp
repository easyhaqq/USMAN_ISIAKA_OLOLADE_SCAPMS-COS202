#include "controller/ReportGenerator.h"

#include "controller/PatientProcessor.h"
#include "controller/SortEngine.h"
#include "utils/DateUtils.h"

#include <iomanip>
#include <map>
#include <sstream>

namespace scapms::controller {

using model::AppointmentStatus;

const std::vector<ReportType>& allReportTypes() {
    static const std::vector<ReportType> types = {
        ReportType::MostAttended, ReportType::PendingAppointments, ReportType::CategoryDistribution,
        ReportType::DailySummary, ReportType::TriageOverview};
    return types;
}

std::string reportName(ReportType type) {
    switch (type) {
        case ReportType::MostAttended:         return "Most Frequently Attended Patients";
        case ReportType::PendingAppointments:  return "Patients with Pending Appointments";
        case ReportType::CategoryDistribution: return "Patient Category Distribution";
        case ReportType::DailySummary:         return "Daily Appointment Summary";
        case ReportType::TriageOverview:       return "Triage Overview (Polymorphic Processing)";
    }
    return "";
}

std::string Report::toText() const {
    std::vector<std::size_t> widths(headers.size(), 0);
    for (std::size_t c = 0; c < headers.size(); ++c) widths[c] = headers[c].size();
    for (const auto& row : rows) {
        for (std::size_t c = 0; c < row.size() && c < widths.size(); ++c) {
            if (row[c].size() > widths[c]) widths[c] = row[c].size();
        }
    }

    std::ostringstream out;
    out << title << '\n' << std::string(title.size(), '=') << "\n\n";
    auto writeRow = [&](const std::vector<std::string>& cells) {
        for (std::size_t c = 0; c < widths.size(); ++c) {
            out << std::left << std::setw(static_cast<int>(widths[c]) + 2) << (c < cells.size() ? cells[c] : "");
        }
        out << '\n';
    };
    writeRow(headers);
    std::size_t lineWidth = 0;
    for (auto w : widths) lineWidth += w + 2;
    out << std::string(lineWidth, '-') << '\n';
    for (const auto& row : rows) writeRow(row);
    if (!summary.empty()) {
        out << '\n';
        for (const auto& line : summary) out << line << '\n';
    }
    return out.str();
}

Report ReportGenerator::generate(ReportType type, const std::string& date) const {
    switch (type) {
        case ReportType::MostAttended:         return mostFrequentlyAttended();
        case ReportType::PendingAppointments:  return pendingAppointments();
        case ReportType::CategoryDistribution: return categoryDistribution();
        case ReportType::DailySummary:         return dailySummary(date);
        case ReportType::TriageOverview:       return triageOverview();
    }
    return {};
}

Report ReportGenerator::mostFrequentlyAttended(std::size_t limit) const {
    struct Attendance { const model::PatientRecord* patient; int completed; int total; };
    std::vector<Attendance> counts;
    for (const auto& patient : db_.patients()) {
        Attendance a{patient.get(), 0, 0};
        for (const auto* appointment : db_.appointmentsFor(patient->id())) {
            ++a.total;
            if (appointment->status() == AppointmentStatus::Completed) ++a.completed;
        }
        if (a.completed > 0) counts.push_back(a);
    }
    // Rank with our own stable merge sort: highest completed count first.
    sortItems(counts, SortAlgorithm::Merge,
              [](const Attendance& x, const Attendance& y) { return x.completed > y.completed; });

    Report report{reportName(ReportType::MostAttended),
                  {"Rank", "Patient ID", "Name", "Category", "Completed Visits", "All Appointments"}, {}, {}};
    for (std::size_t i = 0; i < counts.size() && i < limit; ++i) {
        const auto& a = counts[i];
        report.rows.push_back({std::to_string(i + 1), a.patient->id(), a.patient->name(),
                               a.patient->categoryName(), std::to_string(a.completed), std::to_string(a.total)});
    }
    report.summary.push_back(counts.empty() ? "No completed appointments recorded yet."
                                            : std::to_string(counts.size()) + " patient(s) have completed at least one visit.");
    return report;
}

Report ReportGenerator::pendingAppointments() const {
    std::vector<const model::Appointment*> pending;
    for (const auto& appointment : db_.appointments()) {
        if (appointment.isPending()) pending.push_back(&appointment);
    }
    sortItems(pending, SortAlgorithm::Insertion, [](const model::Appointment* x, const model::Appointment* y) {
        return x->dateTime() < y->dateTime();
    });

    Report report{reportName(ReportType::PendingAppointments),
                  {"Date", "Time", "Appointment", "Patient ID", "Name", "Category", "Service", "Doctor"}, {}, {}};
    const std::string today = utils::date::today();
    int overdue = 0;
    for (const auto* a : pending) {
        const int index = db_.indexOfPatient(a->patientId());
        const auto* patient = index >= 0 ? db_.patients()[static_cast<std::size_t>(index)].get() : nullptr;
        if (a->date() < today) ++overdue;
        report.rows.push_back({a->date(), a->time(), a->id(), a->patientId(),
                               patient ? patient->name() : "(deleted)", patient ? patient->categoryName() : "-",
                               a->service(), a->doctor()});
    }
    report.summary.push_back("Pending appointments: " + std::to_string(pending.size()));
    if (overdue > 0) report.summary.push_back("Overdue (date already passed): " + std::to_string(overdue));
    return report;
}

Report ReportGenerator::categoryDistribution() const {
    std::map<std::string, int> counts{{"General", 0}, {"Emergency", 0}, {"Follow-Up", 0}};
    for (const auto& patient : db_.patients()) ++counts[patient->categoryName()];

    const double total = static_cast<double>(db_.patientCount());
    Report report{reportName(ReportType::CategoryDistribution), {"Category", "Patients", "Share", "Chart"}, {}, {}};
    for (const char* category : {"General", "Emergency", "Follow-Up"}) {
        const int count = counts[category];
        const double share = total > 0 ? 100.0 * count / total : 0.0;
        std::ostringstream percent;
        percent << std::fixed << std::setprecision(1) << share << "%";
        report.rows.push_back({std::string(category) + " Patients", std::to_string(count), percent.str(),
                               std::string(static_cast<std::size_t>(share / 5.0 + 0.5), '#')});
    }
    report.summary.push_back("Total registered patients: " + std::to_string(db_.patientCount()));
    return report;
}

Report ReportGenerator::dailySummary(const std::string& date) const {
    int completed = 0, pending = 0, cancelled = 0;
    std::map<std::string, int> byService;
    std::vector<const model::Appointment*> ofDay;
    for (const auto& a : db_.appointments()) {
        if (a.date() != date) continue;
        ofDay.push_back(&a);
        ++byService[a.service()];
        if (a.status() == AppointmentStatus::Completed) ++completed;
        else if (a.status() == AppointmentStatus::Pending) ++pending;
        else ++cancelled;
    }
    sortItems(ofDay, SortAlgorithm::Insertion,
              [](const model::Appointment* x, const model::Appointment* y) { return x->time() < y->time(); });

    Report report{reportName(ReportType::DailySummary) + " - " + date + " (" + utils::date::dayName(date) + ")",
                  {"Time", "Appointment", "Patient ID", "Service", "Doctor", "Status"}, {}, {}};
    for (const auto* a : ofDay) {
        report.rows.push_back({a->time(), a->id(), a->patientId(), a->service(), a->doctor(), a->statusName()});
    }
    report.summary = {"Total Appointments: " + std::to_string(ofDay.size()),
                      "Completed: " + std::to_string(completed),
                      "Pending: " + std::to_string(pending),
                      "Cancelled: " + std::to_string(cancelled)};
    for (const auto& [service, count] : byService) {
        report.summary.push_back("  " + service + ": " + std::to_string(count));
    }
    return report;
}

Report ReportGenerator::triageOverview() const {
    std::vector<TriageInfo> triage;
    for (const auto& patient : db_.patients()) triage.push_back(processPatient(*patient));
    sortItems(triage, SortAlgorithm::Merge,
              [](const TriageInfo& x, const TriageInfo& y) { return x.priorityLevel < y.priorityLevel; });

    Report report{reportName(ReportType::TriageOverview),
                  {"Priority", "Patient ID", "Name", "Category", "Est. Minutes", "Details", "Scheduling Policy"}, {}, {}};
    int totalMinutes = 0;
    for (const auto& t : triage) {
        totalMinutes += t.consultationMinutes;
        report.rows.push_back({std::to_string(t.priorityLevel) + " - " + t.priorityLabel, t.id, t.name, t.category,
                               std::to_string(t.consultationMinutes), t.details, t.schedulingPolicy});
    }
    report.summary.push_back("Each row was produced by processPatient(const PatientRecord&), which calls "
                             "virtual functions resolved at run time.");
    report.summary.push_back("Estimated total consultation time: " + std::to_string(totalMinutes) + " minutes.");
    return report;
}

} // namespace scapms::controller
