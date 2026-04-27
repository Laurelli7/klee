// exp99: "Cascade Breadth" — A multi-level pipeline where each stage
// independently branches on a different symbolic variable. Within each
// stage, there's a unique function per branch. BFS processes all states
// at each pipeline stage before moving to the next.
//
// The trick: between pipeline stages, there's a MERGE point (all paths
// reconverge). This means the state count DOESN'T explode exponentially.
// BFS explores 4 branches at stage 1, then all reconverge, then 4 at
// stage 2, etc. Total unique functions: 4 stages × 4 branches = 16.
//
// DFS: Goes through one branch per stage (4 unique functions).
// NURS: May jump between stages inefficiently.
// BFS: Processes all branches at each stage in order (16 unique functions).
//
// Expected: BFS = 16 unique functions; DFS = 4; NURS ≈ 8-12
#include "klee/klee.h"
#include <stdint.h>

// Stage 1 functions
__attribute__((noinline)) int S1_a(int x) { return x + 0x1A; }
__attribute__((noinline)) int S1_b(int x) { return x + 0x1B; }
__attribute__((noinline)) int S1_c(int x) { return x + 0x1C; }
__attribute__((noinline)) int S1_d(int x) { return x + 0x1D; }

// Stage 2 functions
__attribute__((noinline)) int S2_a(int x) { return x + 0x2A; }
__attribute__((noinline)) int S2_b(int x) { return x + 0x2B; }
__attribute__((noinline)) int S2_c(int x) { return x + 0x2C; }
__attribute__((noinline)) int S2_d(int x) { return x + 0x2D; }

// Stage 3 functions
__attribute__((noinline)) int S3_a(int x) { return x + 0x3A; }
__attribute__((noinline)) int S3_b(int x) { return x + 0x3B; }
__attribute__((noinline)) int S3_c(int x) { return x + 0x3C; }
__attribute__((noinline)) int S3_d(int x) { return x + 0x3D; }

// Stage 4 functions
__attribute__((noinline)) int S4_a(int x) { return x + 0x4A; }
__attribute__((noinline)) int S4_b(int x) { return x + 0x4B; }
__attribute__((noinline)) int S4_c(int x) { return x + 0x4C; }
__attribute__((noinline)) int S4_d(int x) { return x + 0x4D; }

// Each stage: branch on 2 bits of a symbolic byte → 4 unique functions
// No state explosion because all paths reconverge after each stage
// (no cross-stage interactions)

int main() {
    uint8_t s1, s2, s3, s4;
    uint8_t trap[4]; // trap data to create deep tails
    klee_make_symbolic(&s1, sizeof(s1), "s1");
    klee_make_symbolic(&s2, sizeof(s2), "s2");
    klee_make_symbolic(&s3, sizeof(s3), "s3");
    klee_make_symbolic(&s4, sizeof(s4), "s4");
    klee_make_symbolic(trap, sizeof(trap), "trap");

    int r = 0;

    // Stage 1
    switch (s1 >> 6) {
        case 0: r = S1_a(r); break;
        case 1: r = S1_b(r); break;
        case 2: r = S1_c(r); break;
        case 3: r = S1_d(r); break;
    }

    // Deep trap after stage 1: DFS gets stuck here
    if (trap[0] & 0x01) r++;
    if (trap[0] & 0x02) r++;
    if (trap[0] & 0x04) r++;
    if (trap[0] & 0x08) r++;
    if (trap[0] & 0x10) r++;
    if (trap[0] & 0x20) r++;
    if (trap[0] & 0x40) r++;
    if (trap[0] & 0x80) r++;

    // Stage 2
    switch (s2 >> 6) {
        case 0: r = S2_a(r); break;
        case 1: r = S2_b(r); break;
        case 2: r = S2_c(r); break;
        case 3: r = S2_d(r); break;
    }

    // Deep trap after stage 2
    if (trap[1] & 0x01) r++;
    if (trap[1] & 0x02) r++;
    if (trap[1] & 0x04) r++;
    if (trap[1] & 0x08) r++;
    if (trap[1] & 0x10) r++;
    if (trap[1] & 0x20) r++;
    if (trap[1] & 0x40) r++;
    if (trap[1] & 0x80) r++;

    // Stage 3
    switch (s3 >> 6) {
        case 0: r = S3_a(r); break;
        case 1: r = S3_b(r); break;
        case 2: r = S3_c(r); break;
        case 3: r = S3_d(r); break;
    }

    // Deep trap after stage 3
    if (trap[2] & 0x01) r++;
    if (trap[2] & 0x02) r++;
    if (trap[2] & 0x04) r++;
    if (trap[2] & 0x08) r++;
    if (trap[2] & 0x10) r++;
    if (trap[2] & 0x20) r++;
    if (trap[2] & 0x40) r++;
    if (trap[2] & 0x80) r++;

    // Stage 4
    switch (s4 >> 6) {
        case 0: r = S4_a(r); break;
        case 1: r = S4_b(r); break;
        case 2: r = S4_c(r); break;
        case 3: r = S4_d(r); break;
    }

    // Final deep trap
    if (trap[3] & 0x01) r++;
    if (trap[3] & 0x02) r++;
    if (trap[3] & 0x04) r++;
    if (trap[3] & 0x08) r++;
    if (trap[3] & 0x10) r++;
    if (trap[3] & 0x20) r++;
    if (trap[3] & 0x40) r++;
    if (trap[3] & 0x80) r++;

    return r;
}
