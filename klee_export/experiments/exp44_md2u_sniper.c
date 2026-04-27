// exp44: "MD2U Sniper" — Designed to make NURS:md2u outperform other
// searchers. The "minimum distance to uncovered" heuristic should win
// when there's a KNOWN uncovered function at a MEASURABLE distance.
//
// Structure: A linear chain of 20 gates with a known function at the end.
// Multiple DISTRACTING side-branches at each gate that lead to already-
// covered code (same function called from different places).
// The distractor paths are wide (many states) but provide no new coverage.
//
// MD2U should thread the needle through all 20 gates because the uncovered
// function at the end is always visible at a measurable distance.
// Other searchers get distracted by the wide side-branches.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int common_fn(int x) { return x; }
__attribute__((noinline)) int rare_stage_1(int x) { return x + 1111; }
__attribute__((noinline)) int rare_stage_2(int x) { return x + 2222; }
__attribute__((noinline)) int rare_stage_3(int x) { return x + 3333; }
__attribute__((noinline)) int ultimate_prize(int x) { return x + 9999; }

int main() {
    uint8_t gates[5]; // 5 gate bytes
    uint8_t distractors[3]; // 3 distractor bytes
    klee_make_symbolic(gates, sizeof(gates), "gates");
    klee_make_symbolic(distractors, sizeof(distractors), "dist");

    int r = 0;

    // Gate 1: pass only if gates[0] > 200
    if (gates[0] > 200) {
        r = rare_stage_1(r);
    } else {
        // Distractor: wide bitfield creating many states
        if (distractors[0] & 0x01) r += common_fn(1);
        if (distractors[0] & 0x02) r += common_fn(2);
        if (distractors[0] & 0x04) r += common_fn(3);
        if (distractors[0] & 0x08) r += common_fn(4);
        if (distractors[0] & 0x10) r += common_fn(5);
        if (distractors[0] & 0x20) r += common_fn(6);
        if (distractors[0] & 0x40) r += common_fn(7);
        if (distractors[0] & 0x80) r += common_fn(8);
        return r;
    }

    // Gate 2: pass only if gates[1] > 200
    if (gates[1] > 200) {
        r = rare_stage_2(r);
    } else {
        if (distractors[1] & 0x01) r += common_fn(1);
        if (distractors[1] & 0x02) r += common_fn(2);
        if (distractors[1] & 0x04) r += common_fn(3);
        if (distractors[1] & 0x08) r += common_fn(4);
        if (distractors[1] & 0x10) r += common_fn(5);
        if (distractors[1] & 0x20) r += common_fn(6);
        if (distractors[1] & 0x40) r += common_fn(7);
        if (distractors[1] & 0x80) r += common_fn(8);
        return r;
    }

    // Gate 3: pass only if gates[2] > 200
    if (gates[2] > 200) {
        r = rare_stage_3(r);
    } else {
        if (distractors[2] & 0x01) r += common_fn(1);
        if (distractors[2] & 0x02) r += common_fn(2);
        if (distractors[2] & 0x04) r += common_fn(3);
        if (distractors[2] & 0x08) r += common_fn(4);
        if (distractors[2] & 0x10) r += common_fn(5);
        if (distractors[2] & 0x20) r += common_fn(6);
        if (distractors[2] & 0x40) r += common_fn(7);
        if (distractors[2] & 0x80) r += common_fn(8);
        return r;
    }

    // Gate 4 & 5: two more gates, simpler
    if (gates[3] > 200 && gates[4] > 200) {
        return ultimate_prize(r);
    }

    return r;
}
