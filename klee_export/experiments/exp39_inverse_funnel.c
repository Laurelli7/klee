// exp39: "Inverse Funnel" — Exponentially many paths converge through
// a SINGLE narrow gate to reach valuable coverage. The first branch
// creates 2^16 states. All 2^16 states must pass through gate (x==0x42)
// to reach 4 unique functions behind the gate.
//
// DFS: dives through all 2^16 then hits the gate → wins BIG because
//       it completes paths and frees memory.
// BFS: creates 2^16 pending states, runs out of memory before gate.
// NURS: depends on whether coverage signal can see past the funnel.
//
// This should show DFS >>> everything else.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int prize_alpha(int v) { return v * 7 + 111; }
__attribute__((noinline)) int prize_beta(int v)  { return v * 11 + 222; }
__attribute__((noinline)) int prize_gamma(int v) { return v * 13 + 333; }
__attribute__((noinline)) int prize_delta(int v) { return v * 17 + 444; }

int main() {
    uint8_t noise[2]; // 2 bytes × 8 bits = 16 branches → 2^16 paths
    uint8_t x;
    klee_make_symbolic(noise, sizeof(noise), "noise");
    klee_make_symbolic(&x, sizeof(x), "x");

    // The funnel: 16 independent branches creating 2^16 states
    int acc = 0;
    for (int i = 0; i < 2; i++) {
        if (noise[i] & 0x01) acc++;
        if (noise[i] & 0x02) acc++;
        if (noise[i] & 0x04) acc++;
        if (noise[i] & 0x08) acc++;
        if (noise[i] & 0x10) acc++;
        if (noise[i] & 0x20) acc++;
        if (noise[i] & 0x40) acc++;
        if (noise[i] & 0x80) acc++;
    }

    // THE GATE: Only x == 0x42 gets through
    if (x != 0x42) return acc;

    // Prize zone: 4 unique functions based on accumulator
    if (acc < 4)       return prize_alpha(acc);
    else if (acc < 8)  return prize_beta(acc);
    else if (acc < 12) return prize_gamma(acc);
    else               return prize_delta(acc);
}
