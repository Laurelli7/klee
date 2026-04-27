// exp58: "Phase Transition" — The program's structure CHANGES halfway
// through. Phase 1 is best for BFS (wide, independent branches).
// Phase 2 is best for DFS (deep, sequential chain). No single
// searcher is optimal for both phases.
//
// Structure:
// Phase 1: 3 independent symbolic bytes, each tested at 4 thresholds.
//   12 independent branches = BFS paradise (no state explosion).
// Phase 2: A sequential chain of 8 equality checks where each
//   check's value depends on the result of the previous check.
//   DFS follows the chain; BFS wastes time on infeasible states.
//
// The transition happens at a specific point, creating a "knee" in
// the execution tree.
//
// Expected: default (interleaved) should do best overall because
// it combines random-path + covnew. Pure DFS or pure BFS should
// each fail at one phase. This SHOULD differentiate default from
// individual searchers.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int wide_handler_A(int x) { return x | 0x100; }
__attribute__((noinline)) int wide_handler_B(int x) { return x | 0x200; }
__attribute__((noinline)) int wide_handler_C(int x) { return x | 0x400; }
__attribute__((noinline)) int deep_handler_1(int x) { return x + 11; }
__attribute__((noinline)) int deep_handler_2(int x) { return x + 22; }
__attribute__((noinline)) int deep_handler_3(int x) { return x + 33; }
__attribute__((noinline)) int deep_handler_4(int x) { return x + 44; }
__attribute__((noinline)) int deep_end(int x) { return x + 9999; }

int main() {
    uint8_t wide[3];    // Phase 1: independent
    uint8_t chain_key;  // Phase 2: sequential
    klee_make_symbolic(wide, sizeof(wide), "wide");
    klee_make_symbolic(&chain_key, sizeof(chain_key), "chain_key");

    int result = 0;

    // === PHASE 1: Wide, independent branches ===
    // 3 bytes × 4 thresholds each = 12 branches, all independent
    for (int i = 0; i < 3; i++) {
        if (wide[i] < 0x40)      { if (i == 0) result = wide_handler_A(result);
                                    else if (i == 1) result = wide_handler_B(result);
                                    else result = wide_handler_C(result); }
        else if (wide[i] < 0x80) result += 1;
        else if (wide[i] < 0xC0) result += 2;
        else                     result += 3;
    }

    // === PHASE 2: Deep sequential chain ===
    // Must unlock in order: key bits determine the path
    uint8_t k = chain_key;

    // Gate 1: low 2 bits must be 0b10
    if ((k & 0x03) != 0x02) return result;
    result = deep_handler_1(result);

    // Gate 2: next 2 bits must be 0b01
    if (((k >> 2) & 0x03) != 0x01) return result;
    result = deep_handler_2(result);

    // Gate 3: next 2 bits must be 0b11
    if (((k >> 4) & 0x03) != 0x03) return result;
    result = deep_handler_3(result);

    // Gate 4: top 2 bits must be 0b00
    if (((k >> 6) & 0x03) != 0x00) return result;
    result = deep_handler_4(result);

    // If we pass all 4 gates, key = 0b00_11_01_10 = 0x36
    return deep_end(result);
}
