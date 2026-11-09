#include "utils/IDGenerator.h"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <utility>

namespace scapms::utils {

IDGenerator::IDGenerator(std::string prefix, int width)
    : prefix_(std::move(prefix)), width_(width) {}

std::string IDGenerator::next() {
    return format(++lastNumber_);
}

std::string IDGenerator::peek() const {
    return format(lastNumber_ + 1);
}

void IDGenerator::observe(const std::string& existingId) {
    if (existingId.rfind(prefix_, 0) != 0) return;   // different prefix: ignore
    const std::string digits = existingId.substr(prefix_.size());
    if (digits.empty()) return;
    for (char c : digits) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return;
    }
    const int number = std::stoi(digits);
    if (number > lastNumber_) lastNumber_ = number;
}

void IDGenerator::reset() {
    lastNumber_ = 0;
}

std::string IDGenerator::format(int number) const {
    std::ostringstream out;
    out << prefix_ << std::setw(width_) << std::setfill('0') << number;
    return out.str();
}

} // namespace scapms::utils
