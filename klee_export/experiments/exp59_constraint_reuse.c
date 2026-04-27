// exp59: "Constraint Reuse" — Tests how searchers interact with
// KLEE's constraint/query caching. The same symbolic variable is
// checked repeatedly in different ways, so cache-friendly orderings
// (similar states explored consecutively) benefit from cache hits.
//
// Structure: One symbolic byte x is checked against 8 different
// thresholds in 8 independent functions. The execution order is
// determined by a symbolic selector. Cache-friendly searchers will
// solve x-constraints once and reuse; cache-unfriendly ones repeat.
//
// DFS: sequential ordering, good cache reuse (consecutive similar states)
// BFS: round-robin ordering, worst cache reuse (alternates wildly)
// random-path: random ordering, medium cache reuse
// NURS: depends on which states it picks — coverage-ordered may
//       group similar-constraint states together
//
// Expected: DFS should have lowest solver time (best cache reuse).
// BFS should have highest solver time. This tests SOLVER interaction,
// not coverage.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int check_A(uint8_t x) { return (x < 32)  ? 1 : 0; }
__attribute__((noinline)) int check_B(uint8_t x) { return (x < 64)  ? 2 : 0; }
__attribute__((noinline)) int check_C(uint8_t x) { return (x < 96)  ? 3 : 0; }
__attribute__((noinline)) int check_D(uint8_t x) { return (x < 128) ? 4 : 0; }
__attribute__((noinline)) int check_E(uint8_t x) { return (x < 160) ? 5 : 0; }
__attribute__((noinline)) int check_F(uint8_t x) { return (x < 192) ? 6 : 0; }
__attribute__((noinline)) int check_G(uint8_t x) { return (x < 224) ? 7 : 0; }
__attribute__((noinline)) int check_H(uint8_t x) { return (x < 255) ? 8 : 0; }

int main() {
    uint8_t x;
    uint8_t order[3]; // determines check order
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(order, sizeof(order), "order");

    int result = 0;

    // Same variable x, checked 8 times in different functions
    // Each check creates a fork on x's value range
    result += check_A(x);
    result += check_B(x);
    result += check_C(x);
    result += check_D(x);
    result += check_E(x);
    result += check_F(x);
    result += check_G(x);
    result += check_H(x);

    // A second pass with conditional ordering
    // This creates states that share x constraints with the first pass
    if (order[0] & 0x01) {
        result += check_A(x) * 10;
        result += check_H(x) * 10;
    } else {
        result += check_D(x) * 10;
        result += check_E(x) * 10;
    }

    if (order[1] & 0x01) {
        result += check_B(x) * 100;
    } else {
        result += check_G(x) * 100;
    }

    // Final: specific range of x unlocks special code
    if (x >= 100 && x <= 110) {
        result += 50000;
    }

    return result;
}
