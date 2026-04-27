// exp81: "Mutual Recursion Web" — Two mutually recursive functions
// each dispatching on a fresh symbolic byte. The recursion depth is
// bounded but the CONSTRAINT HISTORY grows exponentially because
// each recursive call adds constraints from BOTH functions.
//
// f(depth, acc) → branches on sym[depth], then calls g()
// g(depth, acc) → branches on sym[depth+1], then calls f()
//
// This creates a "web" where paths through f and g interleave,
// and each path has a unique constraint history. Coverage is
// spread across two functions, so covnew must discover both.
//
// DFS: follows one recursion path deep, slow to backtrack
// BFS: breadth-first on the call tree ≈ O(2^depth) states
// random-path: tree structure poorly matches the call graph
// covnew: should chase both f and g bodies efficiently
// qc: should prefer shallow calls (fewer accumulated constraints)
// md2u: distance to uncovered code changes at each recursion level
//
// Expected: covnew > md2u > qc > DFS for coverage.
//           qc fastest to reach shallow coverage.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int leaf_f(int x) { return x + 0xF000; }
__attribute__((noinline)) int leaf_g(int x) { return x + 0xA000 + 1; }
__attribute__((noinline)) int combine(int a, int b) { return a ^ b; }

int g(int depth, int acc, uint8_t *sym);

int f(int depth, int acc, uint8_t *sym) {
    if (depth >= 5) return leaf_f(acc);

    uint8_t s = sym[depth * 2];
    int left = acc + (s & 0x0F);
    int right = acc - ((s >> 4) & 0x0F);

    if (s & 0x01) left = combine(left, depth);
    if (s & 0x02) right = combine(right, depth);
    if (s & 0x04) left += 100;
    if (s & 0x08) right += 200;

    return g(depth + 1, left + right, sym);
}

int g(int depth, int acc, uint8_t *sym) {
    if (depth >= 5) return leaf_g(acc);

    uint8_t s = sym[depth * 2 + 1];
    int up = acc * 2;
    int down = acc / 2;

    if (s & 0x10) up += 300;
    if (s & 0x20) down += 400;
    if (s & 0x40) up = combine(up, acc);
    if (s & 0x80) down = combine(down, acc);

    return f(depth + 1, up - down, sym);
}

int main() {
    uint8_t sym[12];  // 2 bytes per recursion level, 6 levels
    klee_make_symbolic(sym, sizeof(sym), "sym");

    int result = f(0, 0, sym);

    // Prize for specific recursion outcomes
    if (result == 0x42) return 9999;
    if ((result & 0xFF) == 0) return 8888;
    return result & 0xFFFF;
}
