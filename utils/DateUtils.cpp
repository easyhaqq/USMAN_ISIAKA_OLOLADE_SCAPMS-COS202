#include "utils/DateUtils.h"

#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace scapms::utils::date {

namespace {

std::tm localNow() {
    std::time_t now = std::time(nullptr);
    std::tm result{};
#ifdef _WIN32
    localtime_s(&result, &now);
#else
    localtime_r(&now, &result);
#endif
    return result;
}

bool allDigits(const std::string& text, std::size_t from, std::size_t count) {
    for (std::size_t i = from; i < from + count; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) return false;
    }
    return true;
}

} // namespace

std::string today() {
    std::tm now = localNow();
    std::ostringstream out;
    out << std::put_time(&now, "%Y-%m-%d");
    return out.str();
}

std::string currentTime() {
    std::tm now = localNow();
    std::ostringstream out;
    out << std::put_time(&now, "%H:%M");
    return out.str();
}

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

bool isValidDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    if (!allDigits(date, 0, 4) || !allDigits(date, 5, 2) || !allDigits(date, 8, 2)) return false;

    const int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));
    if (year < 1900 || year > 2100 || month < 1 || month > 12 || day < 1) return false;

    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysInMonth[month - 1];
    if (month == 2 && isLeapYear(year)) maxDay = 29;
    return day <= maxDay;
}

bool isValidTime(const std::string& time) {
    if (time.size() != 5 || time[2] != ':') return false;
    if (!allDigits(time, 0, 2) || !allDigits(time, 3, 2)) return false;
    const int hours = std::stoi(time.substr(0, 2));
    const int minutes = std::stoi(time.substr(3, 2));
    return hours <= 23 && minutes <= 59;
}

// Sakamoto's algorithm. Assumes the date has already been validated.
int dayOfWeek(const std::string& date) {
    static const int offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));
    if (month < 3) year -= 1;
    return (year + year / 4 - year / 100 + year / 400 + offsets[month - 1] + day) % 7;
}

bool isWeekend(const std::string& date) {
    const int day = dayOfWeek(date);
    return day == 0 || day == 6;
}

std::string dayName(const std::string& date) {
    static const char* names[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                                  "Thursday", "Friday", "Saturday"};
    return isValidDate(date) ? names[dayOfWeek(date)] : "";
}

int toMinutes(const std::string& time) {
    return std::stoi(time.substr(0, 2)) * 60 + std::stoi(time.substr(3, 2));
}

} // namespace scapms::utils::date
