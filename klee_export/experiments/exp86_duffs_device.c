// exp86: "Duff's Device Variant" — The canonical example of
// irreducible control flow in C. A switch statement falls through
// into a do-while loop, with the case labels INSIDE the loop body.
//
// The symbolic input determines:
//   1. How many iterations remain (entry point via switch)
//   2. What operation to perform at each step
//
// This is the nastiest legal C control flow. The switch cases
// interleave with the loop body, so the CFG has edges from
// inside the switch to inside the loop and back.
//
// KLEE's process tree is designed for tree-structured CFGs.
// Duff's device is maximally non-tree. This should strongly
// differentiate random-path (which relies on tree structure)
// from other searchers.
//
// Expected: random-path performs WORST due to tree model mismatch.
//           DFS should be best (follows one execution through).
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int op_add(int x) { return x + 7; }
__attribute__((noinline)) int op_mul(int x) { return x * 3; }
__attribute__((noinline)) int op_xor(int x) { return x ^ 0x55; }
__attribute__((noinline)) int op_sub(int x) { return x - 5; }

int main() {
    uint8_t count_sym;  // determines iteration count
    uint8_t ops[8];     // operation choice per iteration
    uint8_t noise;
    klee_make_symbolic(&count_sym, sizeof(count_sym), "count");
    klee_make_symbolic(ops, sizeof(ops), "ops");
    klee_make_symbolic(&noise, sizeof(noise), "noise");

    // Noise
    int result = 0;
    if (noise & 0x01) result++;
    if (noise & 0x02) result++;
    if (noise & 0x04) result++;
    if (noise & 0x08) result++;
    if (noise & 0x10) result++;
    if (noise & 0x20) result++;
    if (noise & 0x40) result++;
    if (noise & 0x80) result++;

    // Duff's device-style: copy with variable alignment entry
    int count = (count_sym & 0x07) + 1; // 1 to 8
    int n = (count + 3) / 4;
    int step = 0;

    switch (count & 0x03) {
        case 0: do { // entry for count%4==0
                    if (ops[step] & 0x03) result = op_add(result);
                    else result = op_sub(result);
                    step++;
                    // fall through
        case 3:     if ((ops[step % 8] >> 2) & 0x03) result = op_mul(result);
                    else result = op_xor(result);
                    step++;
                    // fall through
        case 2:     if ((ops[step % 8] >> 4) & 0x03) result = op_add(result);
                    else result = op_mul(result);
                    step++;
                    // fall through
        case 1:     if ((ops[step % 8] >> 6) & 0x03) result = op_xor(result);
                    else result = op_sub(result);
                    step++;
                } while (--n > 0);
    }

    // Final check: specific result from specific count+ops combo
    if (result == 42 && count == 7) return 99999;
    if ((result & 0xFF) == 0x55) return 88888;
    return result & 0xFFFF;
}
