// exp52: "Array OOB Hunt" — A program with REAL bugs (out-of-bounds
// array accesses) behind specific symbolic conditions. Tests which
// searcher finds the actual errors fastest.
//
// Structure: 3 arrays of different sizes. Symbolic index can go OOB.
// The OOB path is behind a specific condition that only some searchers
// will prioritize. Between the arrays, there's noise branching.
//
// Key differentiator: KLEE generates ERROR states on OOB. Which
// searcher produces the most errors in a time budget?
//
// DFS: may find first error deep, then get stuck
// BFS: may find errors at same depth systematically
// covnew: OOB code IS new coverage, should chase it
// md2u: distance to OOB handler is measurable
//
// Expected: covnew should find errors fastest (OOB = new code).
// DFS unpredictable. BFS systematic but slow.
#include "klee/klee.h"
#include <stdint.h>

int small[4]  = {10, 20, 30, 40};
int medium[8] = {1, 2, 3, 4, 5, 6, 7, 8};
int large[16] = {0};

__attribute__((noinline)) int safe_read(int *arr, int idx, int sz) {
    if (idx >= 0 && idx < sz) return arr[idx];
    return -1;  // sentinel
}

__attribute__((noinline)) int risky_read(int *arr, int idx) {
    return arr[idx];  // NO bounds check — KLEE will flag OOB
}

int main() {
    uint8_t idx1, idx2, idx3;
    uint8_t mode;
    uint8_t noise;
    klee_make_symbolic(&idx1, sizeof(idx1), "idx1");
    klee_make_symbolic(&idx2, sizeof(idx2), "idx2");
    klee_make_symbolic(&idx3, sizeof(idx3), "idx3");
    klee_make_symbolic(&mode, sizeof(mode), "mode");
    klee_make_symbolic(&noise, sizeof(noise), "noise");

    int result = 0;

    // Noise: 4 independent branches
    if (noise & 0x01) result++;
    if (noise & 0x02) result++;
    if (noise & 0x04) result++;
    if (noise & 0x08) result++;

    // Array 1: Only OOB if mode has bit 0 set
    if (mode & 0x01) {
        result += risky_read(small, idx1);  // OOB if idx1 >= 4
    } else {
        result += safe_read(small, idx1, 4);
    }

    // More noise
    if (noise & 0x10) result++;
    if (noise & 0x20) result++;

    // Array 2: Only OOB if mode has bit 1 set AND idx2 is large
    if (mode & 0x02) {
        if (idx2 > 32) {
            result += risky_read(medium, idx2);  // OOB if idx2 >= 8
        } else {
            result += safe_read(medium, idx2, 8);
        }
    }

    // Array 3: OOB behind TWO conditions — harder to reach
    if ((mode & 0x04) && (idx3 > 100)) {
        if (result > 5) {  // depends on noise + previous reads
            result += risky_read(large, idx3);  // OOB if idx3 >= 16
        }
    }

    return result;
}
