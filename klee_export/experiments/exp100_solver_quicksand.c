// exp100: "Solver Quicksand" — Each branch leads to an increasingly
// expensive solver query. BFS touches each branch's first step quickly
// (covering unique code), while DFS and NURS get mired in the first
// branch's deep solver chain.
//
// Structure: 8 branches, each with a unique function at depth 0,
// then progressively harder constraint chains. The constraints use
// multiplication, making KLEE's STP solver work hard.
//
// Critical: all 8 branches share the SAME deep constraint structure
// (using the same solver-heavy function), so covnew sees no "new
// coverage to chase" deep in any branch. But the sheer solver cost
// traps DFS and random searchers.
//
// Expected: BFS covers all 8 unique functions in 8 scheduling rounds,
// before ANY branch goes deep enough to trigger expensive solving.
#include "klee/klee.h"
#include <stdint.h>

// Unique functions — reachable immediately
__attribute__((noinline)) int quick_A(int x) { return x + 0xAA; }
__attribute__((noinline)) int quick_B(int x) { return x + 0xBB; }
__attribute__((noinline)) int quick_C(int x) { return x + 0xCC; }
__attribute__((noinline)) int quick_D(int x) { return x + 0xDD; }
__attribute__((noinline)) int quick_E(int x) { return x + 0xEE; }
__attribute__((noinline)) int quick_F(int x) { return x + 0xFF; }
__attribute__((noinline)) int quick_G(int x) { return x + 0x11; }
__attribute__((noinline)) int quick_H(int x) { return x + 0x22; }

// Solver quicksand — increasingly expensive queries
__attribute__((noinline)) int quicksand(int seed, uint8_t *data, int len) {
    int acc = seed;
    for (int i = 0; i < len; i++) {
        // Linear constraints at first
        if (data[i] > 100) acc += 1;
        if (data[i] < 50) acc += 2;
        // Then quadratic: harder for solver
        if ((uint16_t)data[i] * data[i] > 10000) acc += 4;
        // Then cross-variable: even harder
        if (i > 0 && (uint16_t)data[i] * data[i-1] > 20000) acc += 8;
        // Modular: hardest
        if (i > 0 && data[i] % (data[i-1] | 1) == 0) acc += 16;
    }
    return acc;
}

int main() {
    uint8_t sel;
    uint8_t quicksand_data[6]; // 6 bytes of solver quicksand
    klee_make_symbolic(&sel, sizeof(sel), "sel");
    klee_make_symbolic(quicksand_data, sizeof(quicksand_data), "qs");

    int r = 0;

    // 8-way branch on top 3 bits — unique coverage at depth 1
    switch (sel >> 5) {
        case 0: r = quick_A(sel); break;
        case 1: r = quick_B(sel); break;
        case 2: r = quick_C(sel); break;
        case 3: r = quick_D(sel); break;
        case 4: r = quick_E(sel); break;
        case 5: r = quick_F(sel); break;
        case 6: r = quick_G(sel); break;
        case 7: r = quick_H(sel); break;
    }

    // All branches enter the same quicksand — no new coverage,
    // but the solver spends exponentially more time at each depth
    r = quicksand(r, quicksand_data, 6);
    return r;
}
