#include "controller/SortEngine.h"

namespace scapms::controller {

const std::vector<SortAlgorithm>& allSortAlgorithms() {
    static const std::vector<SortAlgorithm> algorithms = {
        SortAlgorithm::Insertion, SortAlgorithm::Merge, SortAlgorithm::Quick, SortAlgorithm::Selection};
    return algorithms;
}

std::string algorithmName(SortAlgorithm algorithm) {
    switch (algorithm) {
        case SortAlgorithm::Selection: return "Selection Sort";
        case SortAlgorithm::Insertion: return "Insertion Sort";
        case SortAlgorithm::Merge:     return "Merge Sort";
        case SortAlgorithm::Quick:     return "Quick Sort";
    }
    return "";
}

std::string algorithmComplexity(SortAlgorithm algorithm) {
    switch (algorithm) {
        case SortAlgorithm::Selection: return "O(n^2) always, at most n-1 swaps, not stable";
        case SortAlgorithm::Insertion: return "O(n) best, O(n^2) worst, stable, in place";
        case SortAlgorithm::Merge:     return "O(n log n) always, stable, O(n) extra memory";
        case SortAlgorithm::Quick:     return "O(n log n) average, in place, not stable";
    }
    return "";
}

std::string algorithmRationale(SortAlgorithm algorithm) {
    switch (algorithm) {
        case SortAlgorithm::Selection:
            return "Best when moving records is expensive: it performs the fewest swaps. "
                   "Suitable only for small lists because it always makes n(n-1)/2 comparisons.";
        case SortAlgorithm::Insertion:
            return "Ideal for a small clinic list or a list that is already almost sorted "
                   "(e.g. after registering a few new patients): it runs in near-linear time "
                   "and keeps patients with equal keys in their existing order.";
        case SortAlgorithm::Merge:
            return "Guaranteed O(n log n) and stable, so sorting by age or appointment date keeps "
                   "the previous order among ties. The safest choice as the patient list grows.";
        case SortAlgorithm::Quick:
            return "Usually the fastest for large, randomly ordered lists and sorts in place. "
                   "Median-of-three pivoting avoids the slow case on already-sorted data.";
    }
    return "";
}

} // namespace scapms::controller
