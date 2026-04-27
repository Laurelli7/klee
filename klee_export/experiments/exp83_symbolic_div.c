// exp83: "Symbolic Division Cascade" — A pipeline of divisions where
// each stage divides by a SYMBOLIC value, creating an avalanche of
// non-linear constraints. The solver must reason about division
// remainders, zero-checks, and overflow simultaneously.
//
// Structure:
//   Stage 1: x / a  (a is symbolic, implicit a != 0 check)
//   Stage 2: (x / a) / b  (b is symbolic, implicit b != 0 check)
//   Stage 3: ((x / a) / b) % c  (c is symbolic)
//   ...
//
// Each division creates 2 states (zero check + non-zero path)
// but the constraints become QUADRATICALLY harder because each
// new division must reason about the result of the previous one.
//
// DFS: dives deep into one division chain, solver gets harder
// BFS: must handle all pending zero-checks simultaneously
// NURS:qc: should STRONGLY prefer early stages (cheap divisions)
// covnew: may waste time on unreachable combinations
// md2u: confused by the mix of arithmetic and control flow
//
// Expected: qc >>>>> covnew/md2u because solver cost dominates
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int stage_result(int stage, int val) {
    return val + (stage * 1000);
}

int main() {
    uint8_t x;
    uint8_t divisors[5];
    uint8_t noise[2];
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(divisors, sizeof(divisors), "divisors");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int acc = (int)x;
    int reached = 0;

    // Noise preamble: create base state count
    if (noise[0] & 0x01) acc++;
    if (noise[0] & 0x02) acc++;
    if (noise[0] & 0x04) acc++;
    if (noise[0] & 0x08) acc++;
    if (noise[0] & 0x10) acc++;
    if (noise[0] & 0x20) acc++;
    if (noise[0] & 0x40) acc++;
    if (noise[0] & 0x80) acc++;

    // Division cascade — each stage is harder for the solver
    // Stage 0: simple division
    if (divisors[0] != 0) {
        acc = acc / divisors[0];
        reached = stage_result(0, acc);
    }

    // Stage 1: division of division result
    if (divisors[1] != 0) {
        acc = acc / divisors[1];
        reached = stage_result(1, acc);
    }

    // Stage 2: modulo (introduces non-linear remainder constraint)
    if (divisors[2] != 0) {
        int rem = acc % divisors[2];
        if (rem == 0) {
            reached = stage_result(2, acc);
        } else if (rem == 1) {
            reached = stage_result(3, acc);
        }
    }

    // Noise inter-stage
    if (noise[1] & 0x01) acc++;
    if (noise[1] & 0x02) acc++;
    if (noise[1] & 0x04) acc++;
    if (noise[1] & 0x08) acc++;

    // Stage 3: division with range check (combines division + comparison)
    if (divisors[3] != 0) {
        int q = acc / divisors[3];
        if (q > 10 && q < 20) {
            reached = stage_result(4, q);
        }
    }

    // Stage 4: chained modulo (solver nightmare)
    if (divisors[4] != 0 && divisors[3] != 0) {
        int r1 = acc % divisors[3];
        int r2 = acc % divisors[4];
        if (r1 == r2 && r1 != 0) {
            reached = stage_result(5, r1);
        }
    }

    return reached;
}
