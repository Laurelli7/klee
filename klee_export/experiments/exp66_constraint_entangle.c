// exp66: "Constraint Entanglement" — Multiple symbolic variables are
// entangled through arithmetic, then later tested independently.
// The solver must reason about correlations.
//
// Structure: 4 symbolic bytes (a, b, c, d). First, entangle them:
//   sum = a + b
//   diff = a - c
//   xor = b ^ d
// Then branch on sum, diff, xor independently. Each branch leads to
// unique handlers, but the constraint interactions between (a,b) via
// sum and (a,c) via diff create complex solver work.
//
// Additionally, a final "disentanglement" check requires all 4 vars
// to satisfy a specific relationship, which is only feasible for
// certain constraint combinations.
//
// The key differentiator: solver query difficulty varies WILDLY by path.
// Paths that only check 'sum' are cheap. Paths that check sum+diff+xor
// are expensive (3-variable constraint system).
//
// NURS:qc should prefer the cheap single-constraint paths.
// Other searchers should be forced into expensive multi-constraint paths.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int handle_sum_lo(int v)  { return v + 1000; }
__attribute__((noinline)) int handle_sum_hi(int v)  { return v + 2000; }
__attribute__((noinline)) int handle_diff_neg(int v) { return v + 3000; }
__attribute__((noinline)) int handle_diff_pos(int v) { return v + 4000; }
__attribute__((noinline)) int handle_xor_zero(int v) { return v + 5000; }
__attribute__((noinline)) int handle_xor_high(int v) { return v + 6000; }
__attribute__((noinline)) int handle_entangled(int v) { return v + 99999; }

int main() {
    uint8_t a, b, c, d;
    klee_make_symbolic(&a, sizeof(a), "a");
    klee_make_symbolic(&b, sizeof(b), "b");
    klee_make_symbolic(&c, sizeof(c), "c");
    klee_make_symbolic(&d, sizeof(d), "d");

    // Entangle
    uint16_t sum = (uint16_t)a + (uint16_t)b;
    int16_t diff = (int16_t)a - (int16_t)c;
    uint8_t xor_val = b ^ d;

    int result = 0;

    // Branch on sum (cheap: only involves a, b)
    if (sum < 128)       result = handle_sum_lo(result);
    else if (sum > 384)  result = handle_sum_hi(result);
    else                 result += 1;

    // Branch on diff (medium: involves a, c — shares 'a' with sum)
    if (diff < -64)      result = handle_diff_neg(result);
    else if (diff > 64)  result = handle_diff_pos(result);
    else                 result += 2;

    // Branch on xor (medium: involves b, d — shares 'b' with sum)
    if (xor_val == 0)    result = handle_xor_zero(result);
    else if (xor_val > 200) result = handle_xor_high(result);
    else                 result += 3;

    // The killer: JOINT constraint on all 4 variables
    // a + b == 200 AND a - c == 10 AND b ^ d == 0x55
    // This means: b = 200 - a, c = a - 10, d = b ^ 0x55 = (200-a) ^ 0x55
    // Feasible for many values of 'a'.
    if (sum == 200 && diff == 10 && xor_val == 0x55) {
        result = handle_entangled(result);
    }

    return result;
}
