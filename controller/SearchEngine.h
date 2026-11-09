#ifndef SCAPMS_CONTROLLER_SEARCHENGINE_H
#define SCAPMS_CONTROLLER_SEARCHENGINE_H

#include "model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::controller {

enum class SearchField { Id, Name, Category };
enum class SearchAlgorithm { Auto, Linear, Binary, Recursive };

std::string searchAlgorithmName(SearchAlgorithm algorithm);

struct SearchResult {
    std::vector<const model::PatientRecord*> matches;
    std::string algorithmUsed;
    std::string explanation;      // why this algorithm was used
    std::size_t comparisons = 0;
};

// Hand-written searching algorithms. Auto picks the best one for the current
// state of the data: binary search when the vector is sorted on the searched
// field, otherwise linear (ID, category) or recursive (name) search.
class SearchEngine {
public:
    using Patients = std::vector<model::PatientPtr>;

    static SearchResult search(const model::ClinicDatabase& database, SearchField field,
                               const std::string& keyword, SearchAlgorithm algorithm);

    // "pat-7", "7" or "PAT-0007" -> "PAT-0007". Throws SearchException if invalid.
    static std::string normalisePatientId(const std::string& keyword);

    // ---- the algorithms ----
    static int linearSearchById(const Patients& patients, const std::string& id,
                                std::size_t& comparisons);
    static int binarySearchById(const Patients& patients, const std::string& id,
                                std::size_t& comparisons);           // requires sort by ID
    static std::vector<std::size_t> binarySearchByNamePrefix(const Patients& patients,
                                                             const std::string& prefix,
                                                             std::size_t& comparisons); // requires sort by name
    static void recursiveSearchByName(const Patients& patients, const std::string& keyword,
                                      std::size_t index, std::vector<std::size_t>& matches,
                                      std::size_t& comparisons);
    static std::vector<std::size_t> linearSearchByName(const Patients& patients,
                                                       const std::string& keyword,
                                                       std::size_t& comparisons);
    static std::vector<std::size_t> linearSearchByCategory(const Patients& patients,
                                                           model::PatientCategory category,
                                                           std::size_t& comparisons);

private:
    static model::PatientCategory parseCategory(const std::string& keyword);
    static bool containsIgnoreCase(const std::string& text, const std::string& lowerKeyword);
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_SEARCHENGINE_H
