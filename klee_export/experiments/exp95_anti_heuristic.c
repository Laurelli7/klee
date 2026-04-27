// exp95: "Anti-Heuristic" — Designed to confuse NURS:covnew and md2u.
//
// The program has TWO kinds of uncovered code:
// 1. CHEAP coverage: 16 unique functions at depth 1 (via switch)
// 2. EXPENSIVE decoy coverage: unique functions behind DEEP, EXPENSIVE
//    solver chains. These are technically reachable but require solving
//    a chain of 8 dependent constraints.
//
// covnew/md2u: see the expensive decoys as "uncovered nearby" for states
// deep in the solver chain. They'll spend solver time trying to reach
// the decoys while ignoring the cheap depth-1 coverage.
//
// BFS: Systematically explores depth 1 first, hitting all 16 cheap
// functions before going deeper.
//
// DFS: Gets trapped in the first branch's deep solver chain.
//
// random-path: May randomly explore some cheap branches, but also
// wanders into expensive chains.
//
// Expected: BFS > all others
#include "klee/klee.h"
#include <stdint.h>

// Cheap coverage — reachable at depth 1
__attribute__((noinline)) int cheap_00(int x) { return x ^ 0xA0; }
__attribute__((noinline)) int cheap_01(int x) { return x ^ 0xA1; }
__attribute__((noinline)) int cheap_02(int x) { return x ^ 0xA2; }
__attribute__((noinline)) int cheap_03(int x) { return x ^ 0xA3; }
__attribute__((noinline)) int cheap_04(int x) { return x ^ 0xA4; }
__attribute__((noinline)) int cheap_05(int x) { return x ^ 0xA5; }
__attribute__((noinline)) int cheap_06(int x) { return x ^ 0xA6; }
__attribute__((noinline)) int cheap_07(int x) { return x ^ 0xA7; }
__attribute__((noinline)) int cheap_08(int x) { return x ^ 0xA8; }
__attribute__((noinline)) int cheap_09(int x) { return x ^ 0xA9; }
__attribute__((noinline)) int cheap_10(int x) { return x ^ 0xAA; }
__attribute__((noinline)) int cheap_11(int x) { return x ^ 0xAB; }
__attribute__((noinline)) int cheap_12(int x) { return x ^ 0xAC; }
__attribute__((noinline)) int cheap_13(int x) { return x ^ 0xAD; }
__attribute__((noinline)) int cheap_14(int x) { return x ^ 0xAE; }
__attribute__((noinline)) int cheap_15(int x) { return x ^ 0xAF; }

// Expensive decoy coverage — technically reachable via deep chains
__attribute__((noinline)) int decoy_prize_A(int x) { return x * 9999 + 1; }
__attribute__((noinline)) int decoy_prize_B(int x) { return x * 9999 + 2; }
__attribute__((noinline)) int decoy_prize_C(int x) { return x * 9999 + 3; }
__attribute__((noinline)) int decoy_prize_D(int x) { return x * 9999 + 4; }

int main() {
    uint8_t sel;
    uint8_t chain[8]; // 8 bytes for expensive solver chains
    klee_make_symbolic(&sel, sizeof(sel), "sel");
    klee_make_symbolic(chain, sizeof(chain), "chain");

    int r = 0;
    uint8_t which = sel >> 4; // 0..15

    // Cheap coverage at depth 1
    switch (which) {
        case  0: r = cheap_00(sel); break;
        case  1: r = cheap_01(sel); break;
        case  2: r = cheap_02(sel); break;
        case  3: r = cheap_03(sel); break;
        case  4: r = cheap_04(sel); break;
        case  5: r = cheap_05(sel); break;
        case  6: r = cheap_06(sel); break;
        case  7: r = cheap_07(sel); break;
        case  8: r = cheap_08(sel); break;
        case  9: r = cheap_09(sel); break;
        case 10: r = cheap_10(sel); break;
        case 11: r = cheap_11(sel); break;
        case 12: r = cheap_12(sel); break;
        case 13: r = cheap_13(sel); break;
        case 14: r = cheap_14(sel); break;
        case 15: r = cheap_15(sel); break;
    }

    // Expensive solver chains → decoy prizes at the end
    // These create states with "nearby uncovered" (the decoy) that
    // covnew/md2u will prioritize, but solving each step is slow.

    // Chain 1: 8 dependent constraints to reach decoy_prize_A
    if (chain[0] > 100) {
        if (chain[1] + chain[0] > 200) {
            if (chain[2] * 3 > chain[1] + 100) {
                if ((chain[3] ^ chain[2]) > 50) {
                    if (chain[4] > chain[3]) {
                        if (chain[5] + chain[4] + chain[3] > 300) {
                            if (chain[6] > chain[5] + 20) {
                                if (chain[7] == (chain[6] ^ chain[0])) {
                                    r = decoy_prize_A(r);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Chain 2: different constraints to reach decoy_prize_B
    if (chain[0] < 50) {
        if (chain[1] < 30) {
            if (chain[2] + chain[3] < 40) {
                if (chain[4] == chain[0] + chain[1]) {
                    if (chain[5] == chain[2] + chain[3]) {
                        if (chain[6] > 200) {
                            if (chain[7] > 200) {
                                r = decoy_prize_B(r);
                            }
                        }
                    }
                }
            }
        }
    }

    // Chain 3: nonlinear constraints
    if (chain[0] > 200) {
        if (chain[1] > 200) {
            if ((uint16_t)chain[0] * chain[1] > 50000) {
                if (chain[2] < 10) {
                    if (chain[3] < 10) {
                        r = decoy_prize_C(r);
                    }
                }
            }
        }
    }

    // Chain 4: XOR chain
    if ((chain[0] ^ chain[1] ^ chain[2] ^ chain[3]) == 0xAB) {
        if ((chain[4] ^ chain[5] ^ chain[6] ^ chain[7]) == 0xCD) {
            r = decoy_prize_D(r);
        }
    }

    return r;
}
