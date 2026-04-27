// exp60: "Infeasible Maze" — Most branches are UNSATISFIABLE.
// The program has many branches that look reachable in the CFG but
// are infeasible given the constraints accumulated so far.
//
// Structure: 3 symbolic vars (x, y, z). First, tight constraints
// are applied (x < 100, y < 100, z < 100). Then, branches test
// combinations that are mostly infeasible given prior constraints.
// E.g., "if (x > 200)" is infeasible after "x < 100".
//
// Key insight: searchers that eagerly fork on EVERY branch waste
// time creating states that the solver immediately kills.
// DFS: creates one infeasible state, solver kills it, moves on
// BFS: creates ALL infeasible states at a level, solver kills them all
// covnew: chases "new" infeasible branches — total waste
// qc: solver cost of infeasible queries differs from feasible ones
//
// Expected: DFS should win (linear through feasible path).
// BFS/covnew should waste time on infeasible branches.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int feasible_path(int v)   { return v + 1000; }
__attribute__((noinline)) int rare_feasible(int v)    { return v + 2000; }
__attribute__((noinline)) int never_reached(int v)    { return v + 9999; }

int main() {
    uint8_t x, y, z;
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(&y, sizeof(y), "y");
    klee_make_symbolic(&z, sizeof(z), "z");

    int result = 0;

    // Tight initial constraints
    klee_assume(x < 100);
    klee_assume(y < 100);
    klee_assume(z < 100);

    // Layer 1: These branches are feasible
    if (x < 50) result += 1; else result += 2;
    if (y < 50) result += 4; else result += 8;
    if (z < 50) result += 16; else result += 32;

    // Layer 2: Mix of feasible and infeasible
    if (x > 80 && y > 80) {
        // Feasible: x in [81,99], y in [81,99]
        if (z > 80) result = feasible_path(result);
        else        result += 100;
        
        // Infeasible trap: x > 80 but x < 20 simultaneously
        if (x < 20) result = never_reached(result);
    }

    // Infeasible branches that LOOK reachable to the CFG
    if (x > 150) result = never_reached(result);  // infeasible: x < 100
    if (y > 200) result = never_reached(result);   // infeasible: y < 100
    if (x + y > 250) result = never_reached(result); // infeasible: max x+y = 198

    // Subtle: x + y CAN be > 160 (both in [81,99])
    if (x + y > 160) {
        result = rare_feasible(result);
    }

    // Very subtle: this IS feasible if x=99, y=99, z=99 (sum=297 > 250)
    if ((uint16_t)x + (uint16_t)y + (uint16_t)z > 250) {
        result += 5000;
    }

    // Deeply infeasible: requires x > 100 after klee_assume(x < 100)
    if (x > 100 && y > 100 && z > 100) {
        result = never_reached(result);
    }

    return result;
}
