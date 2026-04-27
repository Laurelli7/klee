// exp90: "Symbolic Allocation Pressure" — Different paths allocate
// different amounts of memory. Some paths are "lightweight" (small
// allocations) and some are "heavyweight" (large allocations).
//
// KLEE tracks memory usage per state. States with large allocations
// consume more of KLEE's memory budget, potentially triggering
// early termination. This creates a pressure gradient where
// memory-hungry states are implicitly penalized.
//
// Structure:
//   Path A: allocates sizeof(int) arrays of size 1 (light)
//   Path B: allocates sizeof(int) arrays of size 64 (medium)
//   Path C: allocates sizeof(int) arrays of size 256 (heavy)
//   Each path has unique coverage to discover.
//
// Searchers that spend time on Path C will exhaust KLEE's memory
// budget faster, reducing total exploration time for other paths.
//
// Expected: qc might prefer paths with smaller allocations (fewer
// interations needed). covnew is agnostic but may get blocked by
// memory limits. DFS commits to one path's allocation pattern.
#include "klee/klee.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

__attribute__((noinline)) int process_light(int *buf, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) sum += buf[i];
    return sum + 0x1000;
}
__attribute__((noinline)) int process_medium(int *buf, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) sum += buf[i] * 2;
    return sum + 0x2000;
}
__attribute__((noinline)) int process_heavy(int *buf, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) sum += buf[i] * 3;
    return sum + 0x3000;
}
__attribute__((noinline)) int accumulate(int a, int b) { return a + b; }

int main() {
    uint8_t alloc_choice;
    uint8_t data_seed;
    uint8_t noise;
    klee_make_symbolic(&alloc_choice, sizeof(alloc_choice), "alloc_choice");
    klee_make_symbolic(&data_seed, sizeof(data_seed), "data_seed");
    klee_make_symbolic(&noise, sizeof(noise), "noise");

    int result = 0;

    // Noise preamble
    if (noise & 0x01) result++;
    if (noise & 0x02) result++;
    if (noise & 0x04) result++;
    if (noise & 0x08) result++;
    if (noise & 0x10) result++;
    if (noise & 0x20) result++;
    if (noise & 0x40) result++;
    if (noise & 0x80) result++;

    uint8_t path = alloc_choice & 0x03;
    int *buf;
    int n;

    switch (path) {
        case 0:  // Light path
            n = 4;
            buf = (int*)malloc(n * sizeof(int));
            if (!buf) return -1;
            for (int i = 0; i < n; i++) buf[i] = data_seed + i;
            result = accumulate(result, process_light(buf, n));
            free(buf);
            break;

        case 1:  // Medium path
            n = 32;
            buf = (int*)malloc(n * sizeof(int));
            if (!buf) return -1;
            for (int i = 0; i < n; i++) buf[i] = data_seed * (i + 1);
            result = accumulate(result, process_medium(buf, n));
            free(buf);
            break;

        case 2:  // Heavy path
            n = 128;
            buf = (int*)malloc(n * sizeof(int));
            if (!buf) return -1;
            for (int i = 0; i < n; i++) buf[i] = (data_seed ^ i) + i;
            result = accumulate(result, process_heavy(buf, n));
            free(buf);
            break;

        default:  // Extra heavy: multiple allocations
            for (int j = 0; j < 3; j++) {
                n = 16;
                buf = (int*)malloc(n * sizeof(int));
                if (!buf) return -1;
                for (int i = 0; i < n; i++) buf[i] = data_seed + i + j;
                result = accumulate(result, process_light(buf, n));
                free(buf);
            }
            break;
    }

    // Post-processing: all paths converge
    if (result > 10000) return 1;
    if (result > 5000) return 2;
    if (result > 1000) return 3;
    return result & 0xFF;
}
