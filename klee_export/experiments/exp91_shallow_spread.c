// exp91: "Shallow Spread" — 16 unique functions reachable at depth 1 via
// a 16-way switch on a single symbolic byte. After each unique function,
// the path enters a SOLVER-HEAVY computation (multiplications of symbolic
// values) that creates expensive queries but NO new code coverage.
//
// Strategy: BFS explores all 16 switch arms at depth 1, covering all 16
// unique functions quickly. DFS enters one arm and gets stuck in the
// solver-heavy tail. NURS heuristics may also get distracted because the
// solver-heavy tails create many states with no uncovered instructions
// nearby, diluting the heuristic signal.
//
// Short timeout (10s) ensures DFS can't escape the first tail.
//
// Expected: BFS > all others on CoveredInstr
#include "klee/klee.h"
#include <stdint.h>

// 16 unique functions — each contributes unique CoveredInstr
__attribute__((noinline)) int fn_00(int x) { return x * 2 + 100; }
__attribute__((noinline)) int fn_01(int x) { return x * 3 + 200; }
__attribute__((noinline)) int fn_02(int x) { return x * 5 + 300; }
__attribute__((noinline)) int fn_03(int x) { return x * 7 + 400; }
__attribute__((noinline)) int fn_04(int x) { return x * 11 + 500; }
__attribute__((noinline)) int fn_05(int x) { return x * 13 + 600; }
__attribute__((noinline)) int fn_06(int x) { return x * 17 + 700; }
__attribute__((noinline)) int fn_07(int x) { return x * 19 + 800; }
__attribute__((noinline)) int fn_08(int x) { return x * 23 + 900; }
__attribute__((noinline)) int fn_09(int x) { return x * 29 + 1000; }
__attribute__((noinline)) int fn_10(int x) { return x * 31 + 1100; }
__attribute__((noinline)) int fn_11(int x) { return x * 37 + 1200; }
__attribute__((noinline)) int fn_12(int x) { return x * 41 + 1300; }
__attribute__((noinline)) int fn_13(int x) { return x * 43 + 1400; }
__attribute__((noinline)) int fn_14(int x) { return x * 47 + 1500; }
__attribute__((noinline)) int fn_15(int x) { return x * 53 + 1600; }

// Solver-heavy tail: no new coverage but expensive constraints
__attribute__((noinline)) int heavy_tail(int seed, uint8_t *data, int len) {
    int acc = seed;
    for (int i = 0; i < len; i++) {
        // Each iteration: 8 symbolic branches = exponential paths
        if (data[i] & 0x01) acc += 1;
        if (data[i] & 0x02) acc += 2;
        if (data[i] & 0x04) acc += 4;
        if (data[i] & 0x08) acc += 8;
        if (data[i] & 0x10) acc += 16;
        if (data[i] & 0x20) acc += 32;
        if (data[i] & 0x40) acc += 64;
        if (data[i] & 0x80) acc += 128;
    }
    return acc;
}

int main() {
    uint8_t sel;
    uint8_t tail_data[4]; // 4 bytes = 32 bits = 2^32 paths in tail
    klee_make_symbolic(&sel, sizeof(sel), "sel");
    klee_make_symbolic(tail_data, sizeof(tail_data), "tail");

    int r = 0;
    uint8_t which = sel >> 4; // top 4 bits → 0..15

    switch (which) {
        case  0: r = fn_00(sel); break;
        case  1: r = fn_01(sel); break;
        case  2: r = fn_02(sel); break;
        case  3: r = fn_03(sel); break;
        case  4: r = fn_04(sel); break;
        case  5: r = fn_05(sel); break;
        case  6: r = fn_06(sel); break;
        case  7: r = fn_07(sel); break;
        case  8: r = fn_08(sel); break;
        case  9: r = fn_09(sel); break;
        case 10: r = fn_10(sel); break;
        case 11: r = fn_11(sel); break;
        case 12: r = fn_12(sel); break;
        case 13: r = fn_13(sel); break;
        case 14: r = fn_14(sel); break;
        case 15: r = fn_15(sel); break;
    }

    // ALL branches share the same heavy_tail — no new CoveredInstr here
    r = heavy_tail(r, tail_data, 4);
    return r;
}
