// exp75: "Symbolic Comparator Sort" — A 4-element symbolic sort where
// the comparison function is itself driven by symbolic input.
// This creates an unusually deep constraint tree because each
// comparison fork constrains the relative ordering of elements.
//
// Structure: 4 symbolic uint8 values to sort. Use bubble sort (6 comparisons).
// Each comparison creates a fork (a > b vs a <= b). After sorting,
// unique handlers based on the sorted order.
//
// Key insight: The 6 comparisons create at most 24 total orderings,
// but the constraint COMPLEXITY varies by path. Early comparisons
// are cheap, but later comparisons must also satisfy ALL previous
// ordering constraints. The solver cost grows with each comparison.
//
// DFS: follows one comparison sequence to completion
// BFS: creates 2 states per comparison, 2→4→8→16→32→64
// NURS:qc: should detect increasing query cost in later comparisons
// covnew: same instruction coverage for all comparison outcomes
//
// Expected: DFS completes sorting fast. BFS creates 64 states.
// qc may prefer "already partially sorted" paths (cheaper queries).
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) void swap(uint8_t *a, uint8_t *b) {
    uint8_t t = *a; *a = *b; *b = t;
}

__attribute__((noinline)) int sorted_ascending(int x) { return x + 1000; }
__attribute__((noinline)) int sorted_descending(int x) { return x + 2000; }
__attribute__((noinline)) int sorted_other(int x) { return x + 3000; }
__attribute__((noinline)) int all_equal(int x) { return x + 4000; }

int main() {
    uint8_t arr[4];
    uint8_t noise[3];
    klee_make_symbolic(arr, sizeof(arr), "arr");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int result = 0;

    // Noise amplifier
    for (int i = 0; i < 3; i++) {
        if (noise[i] & 0x01) result++;
        if (noise[i] & 0x02) result++;
        if (noise[i] & 0x04) result++;
        if (noise[i] & 0x08) result++;
        if (noise[i] & 0x10) result++;
        if (noise[i] & 0x20) result++;
        if (noise[i] & 0x40) result++;
        if (noise[i] & 0x80) result++;
    }

    // Bubble sort: 3 passes, 6 comparisons total
    // Pass 1
    if (arr[0] > arr[1]) swap(&arr[0], &arr[1]);
    if (arr[1] > arr[2]) swap(&arr[1], &arr[2]);
    if (arr[2] > arr[3]) swap(&arr[2], &arr[3]);
    // Pass 2
    if (arr[0] > arr[1]) swap(&arr[0], &arr[1]);
    if (arr[1] > arr[2]) swap(&arr[1], &arr[2]);
    // Pass 3
    if (arr[0] > arr[1]) swap(&arr[0], &arr[1]);

    // Classify the sorted result
    if (arr[0] == arr[1] && arr[1] == arr[2] && arr[2] == arr[3]) {
        result = all_equal(result);
    } else if (arr[0] < arr[1] && arr[1] < arr[2] && arr[2] < arr[3]) {
        result = sorted_ascending(result);
    } else {
        // arr is sorted, so strictly ascending or has ties
        // Check if original was descending (arr values are permuted)
        result = sorted_other(result);
    }

    // Deep check: specific sorted order
    if (arr[0] == 0 && arr[3] == 255) {
        result += 50000; // extremes present
    }

    return result;
}
