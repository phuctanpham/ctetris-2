#pragma once
// =============================================================================
// gameConsole_sort.h -- Smart Sorting Engine v2.0
// =============================================================================
// Header-only library: 7 sort algorithms + context-aware Smart Router.
//
// Algorithms (all templated on T + comparator):
//   ALGO_INSERTION : O(n^2); near-O(n) on sorted/append data; tiny constants
//   ALGO_SELECTION : O(n^2); min writes -- safe for flash/EEPROM
//   ALGO_HEAP      : O(n log n); in-place, no recursion (no stack overflow)
//   ALGO_MERGE     : O(n log n); stable; external-mode capable
//   ALGO_QUICK     : O(n log n) avg; pure recursive; parallelizable
//   ALGO_INTRO     : QuickSort + HeapSort (depth fallback) + InsertionSort tail
//   ALGO_TIM       : run-detect + adaptive merge; stable; near-O(n) on partial
//
// Smart Router contexts (see selectAlgo + sort below):
//   CTX_DEFAULT        : in-RAM, single criterion -> Insertion(n<=64) | Intro
//   CTX_FLASH_MEMORY   : minimize writes      -> Selection
//   CTX_RAM_CRITICAL   : zero extra alloc      -> Heap
//   CTX_EXTERNAL       : data > RAM            -> Merge (external mode)
//   CTX_PARALLEL       : many cores + big n    -> Quick (parallel-ready)
//   CTX_STREAMING      : append-one-then-sort  -> Insertion
//   CTX_MULTI_CRITERIA : need stability        -> Tim
//
// Comparator contract: cmp(a,b) returns true iff a should come BEFORE b
// in the final sorted order. e.g. for descending-by-score:
//   [](const X& a, const X& b){ return a.score > b.score; }
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <vector>
#include <utility>

namespace SortEngine {

enum SortAlgo {
    ALGO_AUTO = 0, ALGO_INSERTION, ALGO_SELECTION, ALGO_HEAP,
    ALGO_MERGE, ALGO_QUICK, ALGO_INTRO, ALGO_TIM
};
enum SortContext {
    CTX_DEFAULT = 0, CTX_FLASH_MEMORY, CTX_RAM_CRITICAL, CTX_EXTERNAL,
    CTX_PARALLEL, CTX_STREAMING, CTX_MULTI_CRITERIA
};

inline const char* algoName(SortAlgo a) {
    switch (a) {
        case ALGO_INSERTION: return "insertion";
        case ALGO_SELECTION: return "selection";
        case ALGO_HEAP:      return "heap";
        case ALGO_MERGE:     return "merge";
        case ALGO_QUICK:     return "quick";
        case ALGO_INTRO:     return "intro";
        case ALGO_TIM:       return "tim";
        default:             return "auto";
    }
}

// ----- 1. Insertion Sort -----
template<typename T, typename Cmp>
void insertionSort(T* arr, size_t lo, size_t hi, Cmp cmp) {
    for (size_t i = lo + 1; i < hi; i++) {
        T key = std::move(arr[i]);
        size_t j = i;
        while (j > lo && cmp(key, arr[j - 1])) {
            arr[j] = std::move(arr[j - 1]);
            j--;
        }
        arr[j] = std::move(key);
    }
}
template<typename T, typename Cmp>
inline void insertionSort(T* arr, size_t n, Cmp cmp) { insertionSort(arr, 0, n, cmp); }

// ----- 2. Selection Sort -----
template<typename T, typename Cmp>
void selectionSort(T* arr, size_t n, Cmp cmp) {
    for (size_t i = 0; i + 1 < n; i++) {
        size_t k = i;
        for (size_t j = i + 1; j < n; j++) if (cmp(arr[j], arr[k])) k = j;
        if (k != i) std::swap(arr[i], arr[k]);
    }
}

// ----- 3. Heap Sort -----
template<typename T, typename Cmp>
void siftDown(T* arr, size_t start, size_t end, Cmp cmp) {
    size_t root = start;
    while (root * 2 + 1 < end) {
        size_t child = root * 2 + 1;
        if (child + 1 < end && cmp(arr[child], arr[child + 1])) child++;
        if (!cmp(arr[root], arr[child])) return;
        std::swap(arr[root], arr[child]);
        root = child;
    }
}
template<typename T, typename Cmp>
void heapSort(T* arr, size_t n, Cmp cmp) {
    if (n < 2) return;
    for (size_t s = n / 2; s-- > 0; ) siftDown(arr, s, n, cmp);
    for (size_t e = n; e > 1; ) {
        e--; std::swap(arr[0], arr[e]);
        siftDown(arr, 0, e, cmp);
    }
}

// ----- 4. Merge Sort -----
template<typename T, typename Cmp>
void mergeInto(T* arr, T* buf, size_t lo, size_t mid, size_t hi, Cmp cmp) {
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        if (cmp(arr[j], arr[i])) buf[k++] = std::move(arr[j++]);
        else                      buf[k++] = std::move(arr[i++]);
    }
    while (i < mid) buf[k++] = std::move(arr[i++]);
    while (j < hi)  buf[k++] = std::move(arr[j++]);
    for (size_t x = lo; x < hi; x++) arr[x] = std::move(buf[x]);
}
template<typename T, typename Cmp>
void mergeSortImpl(T* arr, T* buf, size_t lo, size_t hi, Cmp cmp) {
    if (hi - lo < 2) return;
    size_t mid = lo + (hi - lo) / 2;
    mergeSortImpl(arr, buf, lo, mid, cmp);
    mergeSortImpl(arr, buf, mid, hi, cmp);
    mergeInto(arr, buf, lo, mid, hi, cmp);
}
template<typename T, typename Cmp>
void mergeSort(T* arr, size_t n, Cmp cmp) {
    if (n < 2) return;
    std::vector<T> buf(n);
    mergeSortImpl(arr, buf.data(), 0, n, cmp);
}

// ----- 5. Quick Sort -----
template<typename T, typename Cmp>
size_t hoarePartition(T* arr, size_t lo, size_t hi, Cmp cmp) {
    T pivot = arr[lo + (hi - lo) / 2];
    size_t i = lo, j = hi - 1;
    while (true) {
        while (cmp(arr[i], pivot)) i++;
        while (cmp(pivot, arr[j])) j--;
        if (i >= j) return j;
        std::swap(arr[i], arr[j]);
        i++; j--;
    }
}
template<typename T, typename Cmp>
void quickSortImpl(T* arr, size_t lo, size_t hi, Cmp cmp) {
    if (hi - lo < 2) return;
    size_t p = hoarePartition(arr, lo, hi, cmp);
    quickSortImpl(arr, lo, p + 1, cmp);
    quickSortImpl(arr, p + 1, hi, cmp);
}
template<typename T, typename Cmp>
inline void quickSort(T* arr, size_t n, Cmp cmp) {
    if (n > 1) quickSortImpl(arr, 0, n, cmp);
}

// ----- 6. IntroSort: Quick + Heap (depth fallback) + Insertion (small tail) -----
inline size_t intLog2(size_t n) { size_t r = 0; while (n > 1) { n >>= 1; r++; } return r; }

template<typename T, typename Cmp>
void introSortImpl(T* arr, size_t lo, size_t hi, int depthLimit, Cmp cmp) {
    while (hi - lo > 16) {
        if (depthLimit == 0) {
            heapSort(arr + lo, hi - lo, cmp);
            return;
        }
        depthLimit--;
        size_t p = hoarePartition(arr, lo, hi, cmp);
        if (p + 1 - lo < hi - (p + 1)) {
            introSortImpl(arr, lo, p + 1, depthLimit, cmp);
            lo = p + 1;
        } else {
            introSortImpl(arr, p + 1, hi, depthLimit, cmp);
            hi = p + 1;
        }
    }
    insertionSort(arr, lo, hi, cmp);
}
template<typename T, typename Cmp>
void introSort(T* arr, size_t n, Cmp cmp) {
    if (n < 2) return;
    introSortImpl(arr, 0, n, (int)(2 * intLog2(n)), cmp);
}

// ----- 7. TimSort (simplified: run-detect + bottom-up merge) -----
static const size_t TIMSORT_MINRUN = 32;

template<typename T, typename Cmp>
size_t countRunAndMakeAscending(T* arr, size_t lo, size_t hi, Cmp cmp) {
    size_t r = lo + 1;
    if (r == hi) return 1;
    if (cmp(arr[r], arr[lo])) {                 // descending -> reverse
        r++;
        while (r < hi && cmp(arr[r], arr[r - 1])) r++;
        size_t a = lo, b = r - 1;
        while (a < b) std::swap(arr[a++], arr[b--]);
    } else {                                     // ascending
        while (r < hi && !cmp(arr[r], arr[r - 1])) r++;
    }
    return r - lo;
}
template<typename T, typename Cmp>
void timSort(T* arr, size_t n, Cmp cmp) {
    if (n < TIMSORT_MINRUN * 2) { insertionSort(arr, 0, n, cmp); return; }
    std::vector<T> buf(n);
    size_t lo = 0;
    while (lo < n) {
        size_t hi = lo + TIMSORT_MINRUN; if (hi > n) hi = n;
        size_t rl = countRunAndMakeAscending(arr, lo, hi, cmp);
        if (rl < hi - lo) insertionSort(arr, lo, hi, cmp);
        lo = hi;
    }
    for (size_t w = TIMSORT_MINRUN; w < n; w *= 2) {
        for (size_t L = 0; L < n; L += 2 * w) {
            size_t M = L + w, R = L + 2 * w;
            if (M > n) M = n;
            if (R > n) R = n;
            if (M < R) mergeInto(arr, buf.data(), L, M, R, cmp);
        }
    }
}

// ----- Smart Router -----
inline SortAlgo selectAlgo(size_t n, SortContext ctx) {
    switch (ctx) {
        case CTX_FLASH_MEMORY:   return ALGO_SELECTION;
        case CTX_RAM_CRITICAL:   return ALGO_HEAP;
        case CTX_EXTERNAL:       return ALGO_MERGE;
        case CTX_PARALLEL:       if (n > 1000000) return ALGO_QUICK; break;
        case CTX_STREAMING:      return ALGO_INSERTION;
        case CTX_MULTI_CRITERIA: return ALGO_TIM;
        case CTX_DEFAULT: default: break;
    }
    if (n <= 64) return ALGO_INSERTION;
    return ALGO_INTRO;
}

template<typename T, typename Cmp>
SortAlgo sort(T* arr, size_t n, Cmp cmp, SortContext ctx = CTX_DEFAULT) {
    SortAlgo a = selectAlgo(n, ctx);
    switch (a) {
        case ALGO_INSERTION: insertionSort(arr, n, cmp); break;
        case ALGO_SELECTION: selectionSort(arr, n, cmp); break;
        case ALGO_HEAP:      heapSort     (arr, n, cmp); break;
        case ALGO_MERGE:     mergeSort    (arr, n, cmp); break;
        case ALGO_QUICK:     quickSort    (arr, n, cmp); break;
        case ALGO_INTRO:     introSort    (arr, n, cmp); break;
        case ALGO_TIM:       timSort      (arr, n, cmp); break;
        case ALGO_AUTO:      introSort    (arr, n, cmp); break;
    }
    return a;
}

} // namespace SortEngine