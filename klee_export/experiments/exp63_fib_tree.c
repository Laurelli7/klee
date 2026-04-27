// exp63: "Recursive Fibonacci Tree" — Symbolic selection of fibonacci-like
// recursive calls. At each level, input bit determines:
//   bit=0: call ONE recursive function (linear growth)
//   bit=1: call TWO recursive functions (exponential growth)
//
// First 4 bits control the first 4 levels. The "exponential" side
// creates 2^(depth) states at each level, snowballing massively.
//
// DFS: if it hits the exponential side early, it drowns in states
// BFS: creates all states at each level, explodes on exponential side
// random-path: samples uniformly from the tree, should balance
// covnew: each recursive call is "new" (different stack depth)
//
// This tests how searchers handle ASYMMETRIC tree growth where one
// path creates 1 state and the other creates 2^N states.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int leaf_a(int x) { return x + 1111; }
__attribute__((noinline)) int leaf_b(int x) { return x + 2222; }
__attribute__((noinline)) int leaf_c(int x) { return x + 3333; }
__attribute__((noinline)) int leaf_d(int x) { return x + 4444; }

int fiblike(uint8_t bits, int depth, int acc) {
    if (depth >= 4) {
        // Leaf: classify by accumulated value
        if (acc < 100) return leaf_a(acc);
        if (acc < 500) return leaf_b(acc);
        if (acc < 2000) return leaf_c(acc);
        return leaf_d(acc);
    }

    int bit = (bits >> depth) & 1;
    if (bit == 0) {
        // Linear: one recursive call
        return fiblike(bits, depth + 1, acc + depth * 10);
    } else {
        // Exponential: TWO recursive calls (fibonacci-like)
        int left = fiblike(bits, depth + 1, acc + depth * 10 + 1);
        int right = fiblike(bits, depth + 1, acc + depth * 10 + 2);
        return left + right;
    }
}

int main() {
    uint8_t bits;
    uint8_t extra[3]; // additional noise after the recursion
    klee_make_symbolic(&bits, sizeof(bits), "bits");
    klee_make_symbolic(extra, sizeof(extra), "extra");

    int result = fiblike(bits, 0, 0);

    // Post-recursion noise to amplify state differences
    for (int i = 0; i < 3; i++) {
        if (extra[i] & 0x01) result++;
        if (extra[i] & 0x02) result++;
        if (extra[i] & 0x04) result++;
        if (extra[i] & 0x08) result++;
        if (extra[i] & 0x10) result++;
        if (extra[i] & 0x20) result++;
        if (extra[i] & 0x40) result++;
        if (extra[i] & 0x80) result++;
    }

    return result;
}
