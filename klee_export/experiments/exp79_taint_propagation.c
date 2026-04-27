// exp79: "Tainted Propagation" — Simulates taint tracking where a
// symbolic "taint" flows through the program, accumulating constraints.
// Different operations (add, xor, shift) create different constraint
// complexity for the solver.
//
// Structure: One symbolic byte flows through a pipeline of 8 operations.
// At each step, a symbolic selector chooses the operation:
//   0: identity (no constraint change)
//   1: add constant (simple)
//   2: xor with constant (medium)
//   3: left-shift (creates multiplication constraint — expensive)
//
// After the pipeline, the tainted value is checked against multiple
// targets. The solver cost depends heavily on which operations were
// applied.
//
// NURS:qc should prefer paths with identity/add operations (cheap).
// covnew should chase paths with new operation combinations.
// DFS follows one pipeline to completion.
//
// Expected: qc avoids shift-heavy paths. covnew indifferent to cost.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) uint16_t op_identity(uint16_t v) { return v; }
__attribute__((noinline)) uint16_t op_add(uint16_t v) { return v + 0x37; }
__attribute__((noinline)) uint16_t op_xor(uint16_t v) { return v ^ 0xAB; }
__attribute__((noinline)) uint16_t op_shift(uint16_t v) { return v << 1; }

__attribute__((noinline)) int target_zero(int x) { return x + 10000; }
__attribute__((noinline)) int target_magic(int x) { return x + 20000; }
__attribute__((noinline)) int target_high(int x) { return x + 30000; }
__attribute__((noinline)) int target_low(int x) { return x + 40000; }

int main() {
    uint8_t seed;
    uint8_t ops[8];  // 8 operation selectors
    uint8_t noise[2];
    klee_make_symbolic(&seed, sizeof(seed), "seed");
    klee_make_symbolic(ops, sizeof(ops), "ops");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    uint16_t tainted = (uint16_t)seed;
    int result = 0;

    // Noise
    for (int i = 0; i < 2; i++) {
        if (noise[i] & 0x01) result++;
        if (noise[i] & 0x02) result++;
        if (noise[i] & 0x04) result++;
        if (noise[i] & 0x08) result++;
        if (noise[i] & 0x10) result++;
        if (noise[i] & 0x20) result++;
        if (noise[i] & 0x40) result++;
        if (noise[i] & 0x80) result++;
    }

    // Pipeline: 8 operations on tainted value
    for (int i = 0; i < 8; i++) {
        switch (ops[i] & 0x03) {
            case 0: tainted = op_identity(tainted); break;
            case 1: tainted = op_add(tainted); break;
            case 2: tainted = op_xor(tainted); break;
            case 3: tainted = op_shift(tainted); break;
        }
    }

    // Check tainted value against targets
    if (tainted == 0)       return target_zero(result);
    if (tainted == 0xDEAD)  return target_magic(result);
    if (tainted > 0x8000)   return target_high(result);
    if (tainted < 0x0100)   return target_low(result);

    return result;
}
