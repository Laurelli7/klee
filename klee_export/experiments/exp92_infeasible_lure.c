// exp92: "Infeasible Lure" — Deep paths have code that LOOKS uncovered
// from a static CFG perspective but is guarded by unsatisfiable constraints.
// covnew/md2u see "nearby uncovered" deep in the tree and chase it.
// BFS doesn't care — it explores level by level and covers the REAL
// unique code at depth 1.
//
// Structure:
// - Depth 0: 8-way switch on selector → 8 unique functions (real coverage)
// - Each arm: call unique fn, then enter a chain of nested ifs with
//   contradictory constraints. The nested code has unique functions
//   (decoys) that are UNREACHABLE but visible in the CFG.
// - covnew/md2u will see those decoy functions as "uncovered nearby"
//   and spend time trying to reach them (solver grinds on unsat queries).
//
// Expected: BFS covers all 8 real functions quickly; heuristic searchers
// waste time chasing infeasible decoys.
#include "klee/klee.h"
#include <stdint.h>

// Real coverage — reachable at depth 1
__attribute__((noinline)) int real_A(int x) { return x + 10; }
__attribute__((noinline)) int real_B(int x) { return x + 20; }
__attribute__((noinline)) int real_C(int x) { return x + 30; }
__attribute__((noinline)) int real_D(int x) { return x + 40; }
__attribute__((noinline)) int real_E(int x) { return x + 50; }
__attribute__((noinline)) int real_F(int x) { return x + 60; }
__attribute__((noinline)) int real_G(int x) { return x + 70; }
__attribute__((noinline)) int real_H(int x) { return x + 80; }

// Decoy functions — in CFG but unreachable due to contradictions
__attribute__((noinline)) int decoy_1(int x) { return x * 1000 + 1; }
__attribute__((noinline)) int decoy_2(int x) { return x * 1000 + 2; }
__attribute__((noinline)) int decoy_3(int x) { return x * 1000 + 3; }
__attribute__((noinline)) int decoy_4(int x) { return x * 1000 + 4; }
__attribute__((noinline)) int decoy_5(int x) { return x * 1000 + 5; }
__attribute__((noinline)) int decoy_6(int x) { return x * 1000 + 6; }
__attribute__((noinline)) int decoy_7(int x) { return x * 1000 + 7; }
__attribute__((noinline)) int decoy_8(int x) { return x * 1000 + 8; }

int main() {
    uint8_t sel;
    uint8_t a, b, c; // extra symbolic for infeasible chains
    klee_make_symbolic(&sel, sizeof(sel), "sel");
    klee_make_symbolic(&a, sizeof(a), "a");
    klee_make_symbolic(&b, sizeof(b), "b");
    klee_make_symbolic(&c, sizeof(c), "c");

    int r = 0;
    uint8_t which = sel >> 5; // 0..7

    switch (which) {
        case 0: r = real_A(sel); break;
        case 1: r = real_B(sel); break;
        case 2: r = real_C(sel); break;
        case 3: r = real_D(sel); break;
        case 4: r = real_E(sel); break;
        case 5: r = real_F(sel); break;
        case 6: r = real_G(sel); break;
        case 7: r = real_H(sel); break;
    }

    // Infeasible lure chains — look reachable, but constraints contradict
    // The solver must PROVE these are unsat, which costs time
    if (a > 200) {
        if (a < 100) {  // contradicts a > 200
            r = decoy_1(r);
            if (b > 150) {
                if (b < 50) { // contradicts b > 150
                    r = decoy_2(r);
                }
            }
        }
    }

    if (b > 200) {
        if (b < 100) {
            r = decoy_3(r);
            if (c > 150) {
                if (c < 50) {
                    r = decoy_4(r);
                }
            }
        }
    }

    if (c > 200) {
        if (c < 100) {
            r = decoy_5(r);
        }
    }

    // More complex infeasible chains with arithmetic
    if ((uint16_t)a * a > 60000) {
        // a must be > ~245 for uint8 overflow
        if (a < 10) {
            r = decoy_6(r);
        }
    }

    if (a + b > 250) {
        if (a + b < 10) {
            r = decoy_7(r);
        }
    }

    if ((a ^ b) == 0xFF) {
        if (a == b) { // contradicts XOR == 0xFF
            r = decoy_8(r);
        }
    }

    return r;
}
