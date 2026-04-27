// exp20: "Random-path's sweet spot" — A very deep, narrow tree with
// one random fork at each level. Only 1 path is correct at each depth.
// random-path's randomized walk should occasionally find the right path
// while DFS deterministically picks the wrong side first.
#include "klee/klee.h"
#include <stdint.h>

int main() {
    uint8_t choices[20];
    klee_make_symbolic(choices, sizeof(choices), "choices");

    // You need choices[0]==1, then choices[1]==2, ..., choices[19]==20
    // to reach the deepest bug. At each level, the "wrong" side is a dead end.
    int depth = 0;
    for (int i = 0; i < 20; i++) {
        if (choices[i] == (i + 1)) {
            depth++;
        } else {
            // Wrong key: creates a short branch then returns
            if (choices[i] > 100) return depth * 10 + 1;
            return depth * 10;
        }
    }
    // Made it through all 20 levels!
    return -1; // BUG: requires perfect sequence
}
