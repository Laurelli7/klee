// exp54: "Diamond Lattice" — Paths fork then reconverge MULTIPLE times,
// creating a lattice (not a tree). The same program point is reached
// via different constraint histories, creating semantically equivalent
// but syntactically different states.
//
// Structure: 4 stages, each with a 3-way fork that reconverges.
// After reconvergence, a NEW branch checks a DIFFERENT symbolic var.
// Total unique paths = 3^4 = 81, but only ~12 unique coverage outcomes.
//
// Key insight: searchers that track COVERAGE see 12 meaningful choices.
// Searchers that track STATES see 81 choices. This is the divergence.
//
// DFS: explores 81 paths one at a time (slow but complete)
// BFS: tries all 3 forks at each stage, 3→9→27→81 states
// covnew: should deduplicate at reconvergence (only 12 outcomes matter)
// md2u: reconverged states are at same distance, no signal
//
// Expected: covnew should be most efficient (sees deduplication).
// DFS thorough but slow. BFS state-explodes.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int path_left(int v)   { return v + 1; }
__attribute__((noinline)) int path_center(int v)  { return v + 2; }
__attribute__((noinline)) int path_right(int v)   { return v + 3; }

__attribute__((noinline)) int stage1_done(int v) { return v * 10; }
__attribute__((noinline)) int stage2_done(int v) { return v * 10 + 1; }
__attribute__((noinline)) int stage3_done(int v) { return v * 10 + 2; }
__attribute__((noinline)) int stage4_done(int v) { return v * 10 + 3; }

int main() {
    uint8_t keys[4];
    uint8_t gates[4];
    klee_make_symbolic(keys, sizeof(keys), "keys");
    klee_make_symbolic(gates, sizeof(gates), "gates");

    int acc = 0;

    // Stage 1: fork on keys[0] mod 3, then reconverge
    int stage = keys[0] % 3;
    if (stage == 0)      acc = path_left(acc);
    else if (stage == 1) acc = path_center(acc);
    else                 acc = path_right(acc);
    acc = stage1_done(acc);

    // Gate 1: different symbolic var creates NEW fork after reconvergence
    if (gates[0] > 128) acc += 100;

    // Stage 2
    stage = keys[1] % 3;
    if (stage == 0)      acc = path_left(acc);
    else if (stage == 1) acc = path_center(acc);
    else                 acc = path_right(acc);
    acc = stage2_done(acc);

    if (gates[1] > 128) acc += 200;

    // Stage 3
    stage = keys[2] % 3;
    if (stage == 0)      acc = path_left(acc);
    else if (stage == 1) acc = path_center(acc);
    else                 acc = path_right(acc);
    acc = stage3_done(acc);

    if (gates[2] > 128) acc += 300;

    // Stage 4
    stage = keys[3] % 3;
    if (stage == 0)      acc = path_left(acc);
    else if (stage == 1) acc = path_center(acc);
    else                 acc = path_right(acc);
    acc = stage4_done(acc);

    if (gates[3] > 128) acc += 400;

    return acc;
}
