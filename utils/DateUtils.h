#ifndef SCAPMS_UTILS_DATEUTILS_H
#define SCAPMS_UTILS_DATEUTILS_H

#include <string>

// Small, dependency-free date/time helpers. Dates are ISO "YYYY-MM-DD" and
// times are 24-hour "HH:MM", so plain string comparison gives chronological order.
namespace scapms::utils::date {

std::string today();                          // local date, e.g. "2026-11-08"
std::string currentTime();                    // local time, e.g. "14:05"
bool isLeapYear(int year);
bool isValidDate(const std::string& date);
bool isValidTime(const std::string& time);
int  dayOfWeek(const std::string& date);      // 0 = Sunday ... 6 = Saturday
bool isWeekend(const std::string& date);
std::string dayName(const std::string& date); // "Monday", ...
int  toMinutes(const std::string& time);      // "09:30" -> 570

} // namespace scapms::utils::date

#endif // SCAPMS_UTILS_DATEUTILS_H
