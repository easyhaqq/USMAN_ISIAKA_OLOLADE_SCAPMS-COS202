#ifndef SCAPMS_UTILS_IDGENERATOR_H
#define SCAPMS_UTILS_IDGENERATOR_H

#include <string>

namespace scapms::utils {

// Produces sequential, zero-padded IDs such as PAT-0001 or APT-0042.
// observe() keeps the counter ahead of IDs loaded from file or typed by the user.
class IDGenerator {
public:
    explicit IDGenerator(std::string prefix, int width = 4);

    std::string next();               // returns a new ID and advances the counter
    std::string peek() const;         // the ID next() would return
    void observe(const std::string& existingId);
    void reset();

private:
    std::string format(int number) const;

    std::string prefix_;
    int width_;
    int lastNumber_ = 0;
};

} // namespace scapms::utils

#endif // SCAPMS_UTILS_IDGENERATOR_H
