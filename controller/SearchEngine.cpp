#include "controller/SearchEngine.h"

#include "utils/Exceptions.h"
#include "utils/Validator.h"

#include <cctype>

namespace scapms::controller {

namespace v = utils::validate;
using model::SortKey;

std::string searchAlgorithmName(SearchAlgorithm algorithm) {
    switch (algorithm) {
        case SearchAlgorithm::Auto:      return "Auto (best for current data)";
        case SearchAlgorithm::Linear:    return "Linear Search";
        case SearchAlgorithm::Binary:    return "Binary Search";
        case SearchAlgorithm::Recursive: return "Recursive Search";
    }
    return "";
}

// ------------------------------------------------------------------ helpers

std::string SearchEngine::normalisePatientId(const std::string& keyword) {
    std::string value = v::toUpper(v::trimmed(keyword));
    if (value.rfind("PAT-", 0) == 0) value = value.substr(4);
    else if (value.rfind("PAT", 0) == 0) value = value.substr(3);

    if (value.empty() || value.size() > 4) {
        throw utils::SearchException("Invalid Patient ID. Use the format PAT-0001 (or just the number, e.g. 1).");
    }
    for (char c : value) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            throw utils::SearchException("Invalid Patient ID. Use the format PAT-0001 (or just the number, e.g. 1).");
        }
    }
    return "PAT-" + std::string(4 - value.size(), '0') + value;
}

model::PatientCategory SearchEngine::parseCategory(const std::string& keyword) {
    const std::string value = v::toLower(v::trimmed(keyword));
    const std::string general = "general", emergency = "emergency";
    if (general.rfind(value, 0) == 0) return model::PatientCategory::General;
    if (emergency.rfind(value, 0) == 0) return model::PatientCategory::Emergency;
    if (std::string("follow-up").rfind(value, 0) == 0 || std::string("followup").rfind(value, 0) == 0) {
        return model::PatientCategory::FollowUp;
    }
    throw utils::SearchException("Unknown category '" + keyword + "'. Use General, Emergency or Follow-Up.");
}

bool SearchEngine::containsIgnoreCase(const std::string& text, const std::string& lowerKeyword) {
    return v::toLower(text).find(lowerKeyword) != std::string::npos;
}

// --------------------------------------------------------------- algorithms

int SearchEngine::linearSearchById(const Patients& patients, const std::string& id,
                                   std::size_t& comparisons) {
    for (std::size_t i = 0; i < patients.size(); ++i) {
        ++comparisons;
        if (patients[i]->id() == id) return static_cast<int>(i);
    }
    return -1;
}

int SearchEngine::binarySearchById(const Patients& patients, const std::string& id,
                                   std::size_t& comparisons) {
    std::size_t low = 0, high = patients.size();      // search window [low, high)
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        ++comparisons;
        const std::string& current = patients[mid]->id();
        if (current == id) return static_cast<int>(mid);
        if (current < id) low = mid + 1;
        else              high = mid;
    }
    return -1;
}

std::vector<std::size_t> SearchEngine::binarySearchByNamePrefix(const Patients& patients,
                                                                const std::string& prefix,
                                                                std::size_t& comparisons) {
    // Binary search for the first name >= prefix (lower bound) ...
    std::size_t low = 0, high = patients.size();
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        ++comparisons;
        if (v::toLower(patients[mid]->name()) < prefix) low = mid + 1;
        else                                            high = mid;
    }
    // ... then all names starting with the prefix are next to each other.
    std::vector<std::size_t> matches;
    for (std::size_t i = low; i < patients.size(); ++i) {
        ++comparisons;
        if (v::toLower(patients[i]->name()).rfind(prefix, 0) != 0) break;
        matches.push_back(i);
    }
    return matches;
}

void SearchEngine::recursiveSearchByName(const Patients& patients, const std::string& keyword,
                                         std::size_t index, std::vector<std::size_t>& matches,
                                         std::size_t& comparisons) {
    if (index >= patients.size()) return;              // base case: past the last record
    ++comparisons;
    if (containsIgnoreCase(patients[index]->name(), keyword)) matches.push_back(index);
    recursiveSearchByName(patients, keyword, index + 1, matches, comparisons);  // recursive case
}

std::vector<std::size_t> SearchEngine::linearSearchByName(const Patients& patients,
                                                          const std::string& keyword,
                                                          std::size_t& comparisons) {
    std::vector<std::size_t> matches;
    for (std::size_t i = 0; i < patients.size(); ++i) {
        ++comparisons;
        if (containsIgnoreCase(patients[i]->name(), keyword)) matches.push_back(i);
    }
    return matches;
}

std::vector<std::size_t> SearchEngine::linearSearchByCategory(const Patients& patients,
                                                              model::PatientCategory category,
                                                              std::size_t& comparisons) {
    std::vector<std::size_t> matches;
    for (std::size_t i = 0; i < patients.size(); ++i) {
        ++comparisons;
        if (patients[i]->category() == category) matches.push_back(i);
    }
    return matches;
}

// ------------------------------------------------------------ entry point

SearchResult SearchEngine::search(const model::ClinicDatabase& database, SearchField field,
                                  const std::string& keyword, SearchAlgorithm algorithm) {
    if (v::trimmed(keyword).empty()) throw utils::SearchException("Please enter a search keyword.");

    const Patients& patients = database.patients();
    SearchResult result;
    std::vector<std::size_t> indices;

    switch (field) {
        case SearchField::Id: {
            const std::string id = normalisePatientId(keyword);
            const bool sortedById = database.sortedBy() == SortKey::Id;
            if (algorithm == SearchAlgorithm::Recursive) {
                throw utils::SearchException("Recursive search is provided for patient names. "
                                             "Use Auto, Linear or Binary search for IDs.");
            }
            if (algorithm == SearchAlgorithm::Binary && !sortedById) {
                throw utils::SearchException("Binary search needs the records sorted by Patient ID. "
                                             "Sort by Patient ID first, or choose Auto/Linear.");
            }
            const bool useBinary = algorithm == SearchAlgorithm::Binary ||
                                   (algorithm == SearchAlgorithm::Auto && sortedById);
            const int index = useBinary ? binarySearchById(patients, id, result.comparisons)
                                        : linearSearchById(patients, id, result.comparisons);
            if (index >= 0) indices.push_back(static_cast<std::size_t>(index));
            result.algorithmUsed = useBinary ? "Binary Search" : "Linear Search";
            result.explanation = useBinary
                ? "Records are sorted by Patient ID, so the search halves the list at each step (O(log n))."
                : "Records are not sorted by Patient ID, so each record is checked in turn (O(n)).";
            break;
        }
        case SearchField::Name: {
            const std::string lowered = v::toLower(v::trimmed(keyword));
            const bool sortedByName = database.sortedBy() == SortKey::Name;
            if (algorithm == SearchAlgorithm::Binary && !sortedByName) {
                throw utils::SearchException("Binary search needs the records sorted by Patient Name. "
                                             "Sort by Patient Name first, or choose Auto/Recursive/Linear.");
            }
            if (algorithm == SearchAlgorithm::Binary ||
                (algorithm == SearchAlgorithm::Auto && sortedByName)) {
                indices = binarySearchByNamePrefix(patients, lowered, result.comparisons);
                result.algorithmUsed = "Binary Search";
                result.explanation = "Records are sorted by name, so binary search finds names "
                                     "that START with the keyword in O(log n).";
            } else if (algorithm == SearchAlgorithm::Linear) {
                indices = linearSearchByName(patients, lowered, result.comparisons);
                result.algorithmUsed = "Linear Search";
                result.explanation = "Each name is checked in turn for the keyword (O(n)).";
            } else {
                recursiveSearchByName(patients, lowered, 0, indices, result.comparisons);
                result.algorithmUsed = "Recursive Search";
                result.explanation = "Records are not sorted by name: the recursive search checks record i "
                                     "and then calls itself for record i+1 (names CONTAINING the keyword).";
            }
            break;
        }
        case SearchField::Category: {
            if (algorithm == SearchAlgorithm::Binary || algorithm == SearchAlgorithm::Recursive) {
                throw utils::SearchException("Category search returns many records, so only Auto/Linear "
                                             "search applies.");
            }
            indices = linearSearchByCategory(patients, parseCategory(keyword), result.comparisons);
            result.algorithmUsed = "Linear Search";
            result.explanation = "A category matches many patients, so every record is checked (O(n)).";
            break;
        }
    }

    for (std::size_t index : indices) result.matches.push_back(patients[index].get());
    return result;
}

} // namespace scapms::controller
