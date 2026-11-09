#ifndef SCAPMS_CONTROLLER_SORTENGINE_H
#define SCAPMS_CONTROLLER_SORTENGINE_H

#include <chrono>
#include <string>
#include <utility>
#include <vector>

// Hand-written sorting algorithms (no std::sort). They are templates so they can
// sort any vector, including std::vector<std::unique_ptr<PatientRecord>>.
// Each algorithm takes a "less" comparator and records statistics for the GUI.
namespace scapms::controller {

enum class SortAlgorithm { Selection, Insertion, Merge, Quick };

struct SortStats {
    std::size_t comparisons = 0;
    std::size_t moves = 0;          // element swaps / moves
    double milliseconds = 0.0;
};

const std::vector<SortAlgorithm>& allSortAlgorithms();
std::string algorithmName(SortAlgorithm algorithm);
std::string algorithmComplexity(SortAlgorithm algorithm);
std::string algorithmRationale(SortAlgorithm algorithm);   // when this algorithm is appropriate

namespace sorting {

// Selection sort: repeatedly select the smallest remaining element.
// O(n^2) comparisons but at most n-1 swaps. Not stable.
template <typename T, typename Less>
void selectionSort(std::vector<T>& items, Less& less, SortStats& stats) {
    const std::size_t n = items.size();
    for (std::size_t i = 0; i + 1 < n; ++i) {
        std::size_t smallest = i;
        for (std::size_t j = i + 1; j < n; ++j) {
            if (less(items[j], items[smallest])) smallest = j;
        }
        if (smallest != i) {
            std::swap(items[i], items[smallest]);
            ++stats.moves;
        }
    }
}

// Insertion sort: grow a sorted prefix by inserting one element at a time.
// O(n^2) worst case, O(n) on nearly-sorted data. Stable.
template <typename T, typename Less>
void insertionSort(std::vector<T>& items, Less& less, SortStats& stats) {
    for (std::size_t i = 1; i < items.size(); ++i) {
        T key = std::move(items[i]);
        std::size_t j = i;
        while (j > 0 && less(key, items[j - 1])) {
            items[j] = std::move(items[j - 1]);
            ++stats.moves;
            --j;
        }
        items[j] = std::move(key);
    }
}

// Merge sort helper: sorts the half-open range [low, high) recursively.
template <typename T, typename Less>
void mergeSortRange(std::vector<T>& items, std::vector<T>& buffer, std::size_t low,
                    std::size_t high, Less& less, SortStats& stats) {
    if (high - low < 2) return;                      // base case: 0 or 1 element
    const std::size_t mid = low + (high - low) / 2;
    mergeSortRange(items, buffer, low, mid, less, stats);
    mergeSortRange(items, buffer, mid, high, less, stats);

    std::size_t left = low, right = mid, out = low;
    while (left < mid && right < high) {
        // Take from the right only if strictly smaller: equal keys keep their order (stable).
        if (less(items[right], items[left])) buffer[out++] = std::move(items[right++]);
        else                                 buffer[out++] = std::move(items[left++]);
    }
    while (left < mid)   buffer[out++] = std::move(items[left++]);
    while (right < high) buffer[out++] = std::move(items[right++]);
    for (std::size_t k = low; k < high; ++k) items[k] = std::move(buffer[k]);
    stats.moves += high - low;
}

// Merge sort: divide and conquer. Always O(n log n), stable, needs O(n) extra memory.
template <typename T, typename Less>
void mergeSort(std::vector<T>& items, Less& less, SortStats& stats) {
    std::vector<T> buffer(items.size());
    mergeSortRange(items, buffer, 0, items.size(), less, stats);
}

// Quick sort helper: sorts the inclusive range [low, high].
// Median-of-three pivot avoids the O(n^2) case on already-sorted input, and
// recursing only into the smaller partition keeps the stack depth O(log n).
template <typename T, typename Less>
void quickSortRange(std::vector<T>& items, std::size_t low, std::size_t high, Less& less,
                    SortStats& stats) {
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (less(items[mid], items[low]))  { std::swap(items[mid], items[low]);  ++stats.moves; }
        if (less(items[high], items[low])) { std::swap(items[high], items[low]); ++stats.moves; }
        if (less(items[mid], items[high])) { std::swap(items[mid], items[high]); ++stats.moves; }
        // items[high] now holds the median of the three, used as the pivot (Lomuto partition).

        std::size_t store = low;
        for (std::size_t j = low; j < high; ++j) {
            if (less(items[j], items[high])) {
                std::swap(items[store], items[j]);
                ++stats.moves;
                ++store;
            }
        }
        std::swap(items[store], items[high]);
        ++stats.moves;

        if (store - low < high - store) {
            if (store > low) quickSortRange(items, low, store - 1, less, stats);
            low = store + 1;
        } else {
            quickSortRange(items, store + 1, high, less, stats);
            if (store == 0) break;
            high = store - 1;
        }
    }
}

// Quick sort: divide and conquer, in place. O(n log n) on average. Not stable.
template <typename T, typename Less>
void quickSort(std::vector<T>& items, Less& less, SortStats& stats) {
    if (items.size() > 1) quickSortRange(items, 0, items.size() - 1, less, stats);
}

} // namespace sorting

// Runs the chosen algorithm, counting comparisons and timing the sort.
template <typename T, typename Less>
SortStats sortItems(std::vector<T>& items, SortAlgorithm algorithm, Less less) {
    SortStats stats;
    auto countingLess = [&stats, &less](const T& a, const T& b) {
        ++stats.comparisons;
        return less(a, b);
    };

    const auto start = std::chrono::steady_clock::now();
    switch (algorithm) {
        case SortAlgorithm::Selection: sorting::selectionSort(items, countingLess, stats); break;
        case SortAlgorithm::Insertion: sorting::insertionSort(items, countingLess, stats); break;
        case SortAlgorithm::Merge:     sorting::mergeSort(items, countingLess, stats);     break;
        case SortAlgorithm::Quick:     sorting::quickSort(items, countingLess, stats);     break;
    }
    const auto end = std::chrono::steady_clock::now();
    stats.milliseconds = std::chrono::duration<double, std::milli>(end - start).count();
    return stats;
}

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_SORTENGINE_H
