// exp67: "Conditional Loop Depth" — A nested loop where the inner
// loop bound depends on a symbolic variable. Different values of the
// bound create paths of WILDLY different lengths (1 to 256 iterations).
//
// After the loop, unique code is reachable. DFS commits to one bound
// value and either finishes quickly (bound=1) or drowns (bound=256).
// BFS tries all bound values at depth 0, creating 256 states.
//
// Key insight: the loop MULTIPLIES forking. If there are K branches
// per iteration and the loop runs for N iterations, total states = K^N.
// With 2 branches per iteration: bound=1 → 2 states, bound=8 → 256 states.
//
// Expected: DFS with bound=1 is fast; DFS with bound=255 drowns.
// BFS creates 256 different-depth subtrees.
// covnew should prefer bound values with novel code (loop body is shared).
// random-path should sample diverse bounds.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int loop_body_if(int x) { return x + 7; }
__attribute__((noinline)) int loop_body_else(int x) { return x + 3; }
__attribute__((noinline)) int after_loop(int x) { return x + 10000; }
__attribute__((noinline)) int deep_bonus(int x) { return x + 50000; }

int main() {
    uint8_t bound;     // loop iteration count
    uint8_t toggle;    // controls per-iteration branching
    klee_make_symbolic(&bound, sizeof(bound), "bound");
    klee_make_symbolic(&toggle, sizeof(toggle), "toggle");

    // Clamp bound to [1, 10] — enough to create significant state variation
    // without making any single path infinitely long
    uint8_t n = (bound % 10) + 1;  // 1 to 10

    int result = 0;

    // The loop: 'n' iterations, each has a symbolic branch via toggle bits
    // Since toggle has 8 bits but we use up to 10 iterations,
    // iterations 8-10 reuse bits, creating state sharing
    for (uint8_t i = 0; i < n; i++) {
        uint8_t bit = (toggle >> (i & 7)) & 1;
        if (bit) result = loop_body_if(result);
        else     result = loop_body_else(result);
    }

    result = after_loop(result);

    // Bonus: only reachable with specific bound
    if (n == 7 && result > 50) {
        result = deep_bonus(result);
    }

    return result;
}
