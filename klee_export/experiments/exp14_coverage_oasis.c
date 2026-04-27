// exp14: "Coverage Oasis" — scattered islands of new code behind gates.
// NURS:covnew should excel: each "oasis" provides coverage novelty,
// drawing the searcher toward unexplored code regions.
// Other searchers will waste time in the "desert" (already-covered code).
#include "klee/klee.h"
#include <stdint.h>

// Each function is a unique "oasis" of coverage
__attribute__((noinline)) int oasis_a(int x) { return x * 2 + 1; }
__attribute__((noinline)) int oasis_b(int x) { return x * 3 + 2; }
__attribute__((noinline)) int oasis_c(int x) { return x * 5 + 3; }
__attribute__((noinline)) int oasis_d(int x) { return x * 7 + 4; }
__attribute__((noinline)) int oasis_e(int x) { return x * 11 + 5; }
__attribute__((noinline)) int oasis_f(int x) { return x * 13 + 6; }
__attribute__((noinline)) int oasis_g(int x) { return x * 17 + 7; }
__attribute__((noinline)) int oasis_h(int x) { return x * 19 + 8; }

int main() {
    uint8_t gate[4];
    uint8_t desert[4]; // creates the "desert" of paths between oases
    klee_make_symbolic(gate, sizeof(gate), "gate");
    klee_make_symbolic(desert, sizeof(desert), "desert");

    int result = 0;

    // Desert: 4 bytes × 3 bits = 12 branches creating 2^12 = 4096 paths of SAME code
    for (int i = 0; i < 4; i++) {
        if (desert[i] & 0x01) result++;
        if (desert[i] & 0x02) result++;
        if (desert[i] & 0x04) result++;
    }

    // Oases: 8 gates, each unlocks a unique function (= new coverage)
    // Gates are checked AFTER the desert, so states must traverse it first
    if (gate[0] == 0xAA) result = oasis_a(result);
    if (gate[0] == 0xBB) result = oasis_b(result);
    if (gate[1] == 0xCC) result = oasis_c(result);
    if (gate[1] == 0xDD) result = oasis_d(result);
    if (gate[2] == 0xEE) result = oasis_e(result);
    if (gate[2] == 0xFF) result = oasis_f(result);
    if (gate[3] == 0x11) result = oasis_g(result);
    if (gate[3] == 0x22) result = oasis_h(result);

    return result & 0xFF;
}
