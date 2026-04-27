// exp87: "Convergent-Divergent Pipeline" — The hourglass pattern.
//
//   Phase 1: DIVERGE — N symbolic bytes create 2^N paths
//   Phase 2: CONVERGE — all paths execute the same single function
//   Phase 3: DIVERGE AGAIN — new branches whose feasibility depends
//            on constraints accumulated in Phase 1
//
// At the convergence point, covnew sees "nothing new" because the
// code was already covered. But the constraints from Phase 1 determine
// which Phase 3 branches are feasible. States that look identical
// from a coverage perspective have different constraint-determined futures.
//
// This tests whether searchers can differentiate states that have
// identical coverage but different symbolic futures.
//
// covnew: confused at convergence (all states look "covered")
// md2u: confused (Phase 3 code is equidistant for all states)
// qc: may differentiate by constraint complexity from Phase 1
// DFS: follows one path all the way through
//
// Expected: DFS > qc > covnew ≈ md2u for post-convergence coverage.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int diverge_A(int x) { return x + 0xA; }
__attribute__((noinline)) int diverge_B(int x) { return x + 0xB; }
__attribute__((noinline)) int diverge_C(int x) { return x + 0xC; }
__attribute__((noinline)) int diverge_D(int x) { return x + 0xD; }
__attribute__((noinline)) int converge(int x) { return x * 3 + 7; }
__attribute__((noinline)) int post_lo(int x) { return x + 1000; }
__attribute__((noinline)) int post_mid(int x) { return x + 2000; }
__attribute__((noinline)) int post_hi(int x) { return x + 3000; }
__attribute__((noinline)) int post_extreme(int x) { return x + 4000; }

int main() {
    uint8_t phase1[2];  // divergence bytes
    uint8_t noise;
    klee_make_symbolic(phase1, sizeof(phase1), "phase1");
    klee_make_symbolic(&noise, sizeof(noise), "noise");

    int acc = 0;

    // Phase 1: DIVERGE — accumulate constraints
    if (phase1[0] & 0x01) acc = diverge_A(acc);
    if (phase1[0] & 0x02) acc = diverge_B(acc);
    if (phase1[0] & 0x04) acc = diverge_C(acc);
    if (phase1[0] & 0x08) acc = diverge_D(acc);
    if (phase1[0] & 0x10) acc += 0x10;
    if (phase1[0] & 0x20) acc += 0x20;
    if (phase1[0] & 0x40) acc += 0x40;
    if (phase1[0] & 0x80) acc += 0x80;

    if (phase1[1] & 0x01) acc += 1;
    if (phase1[1] & 0x02) acc += 2;
    if (phase1[1] & 0x04) acc += 4;
    if (phase1[1] & 0x08) acc += 8;
    if (phase1[1] & 0x10) acc += 16;
    if (phase1[1] & 0x20) acc += 32;
    if (phase1[1] & 0x40) acc += 64;
    if (phase1[1] & 0x80) acc += 128;

    // Phase 2: CONVERGE — everyone runs the same code
    acc = converge(acc);

    // Noise at convergence point
    if (noise & 0x01) acc++;
    if (noise & 0x02) acc++;
    if (noise & 0x04) acc++;
    if (noise & 0x08) acc++;

    // Phase 3: DIVERGE AGAIN — but feasibility depends on Phase 1
    // These comparisons depend on the ACCUMULATED value of acc,
    // which is determined by Phase 1's symbolic choices.
    // The solver must reason backwards through the converge() function.
    if (acc > 0 && acc < 100) {
        acc = post_lo(acc);
    } else if (acc >= 100 && acc < 500) {
        acc = post_mid(acc);
    } else if (acc >= 500 && acc < 2000) {
        acc = post_hi(acc);
    } else {
        acc = post_extreme(acc);
    }

    // Deep prize
    if (acc == 2042) return 99999;
    return acc & 0xFFFF;
}
