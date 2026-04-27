// exp96: "Layered Breadth" — Coverage is spread across MULTIPLE depth
// levels but requires visiting ALL siblings at each level before the
// next level unlocks new functions.
//
// Structure: 3 depth layers, each with 4 unique functions gated by
// independent conditions. Between layers, a SHARED function creates
// many states. BFS naturally explores all 4 siblings at layer 1,
// then all at layer 2, then layer 3. DFS goes through one path from
// layer 1 to 3, getting only 3 unique functions instead of 12.
//
// Key innovation: Each layer's conditions are on DIFFERENT symbolic
// bytes, so the layers are truly independent — you don't need to
// solve layer 1 to reach layer 2. The "gate" between layers is just
// sequential code order.
//
// Expected: BFS covers all 12 unique functions; DFS covers ~3-4.
#include "klee/klee.h"
#include <stdint.h>

// Layer 1 functions
__attribute__((noinline)) int L1_a(int x) { return x + 1001; }
__attribute__((noinline)) int L1_b(int x) { return x + 1002; }
__attribute__((noinline)) int L1_c(int x) { return x + 1003; }
__attribute__((noinline)) int L1_d(int x) { return x + 1004; }

// Layer 2 functions
__attribute__((noinline)) int L2_a(int x) { return x + 2001; }
__attribute__((noinline)) int L2_b(int x) { return x + 2002; }
__attribute__((noinline)) int L2_c(int x) { return x + 2003; }
__attribute__((noinline)) int L2_d(int x) { return x + 2004; }

// Layer 3 functions
__attribute__((noinline)) int L3_a(int x) { return x + 3001; }
__attribute__((noinline)) int L3_b(int x) { return x + 3002; }
__attribute__((noinline)) int L3_c(int x) { return x + 3003; }
__attribute__((noinline)) int L3_d(int x) { return x + 3004; }

// Shared inter-layer function — creates state explosion
__attribute__((noinline)) int interleave(int acc, uint8_t data) {
    if (data & 0x01) acc += 1;
    if (data & 0x02) acc += 2;
    if (data & 0x04) acc += 4;
    if (data & 0x08) acc += 8;
    if (data & 0x10) acc += 16;
    if (data & 0x20) acc += 32;
    if (data & 0x40) acc += 64;
    if (data & 0x80) acc += 128;
    return acc;
}

int main() {
    uint8_t s1, s2, s3;  // selectors for each layer
    uint8_t pad[4];       // padding for state explosion between layers
    klee_make_symbolic(&s1, sizeof(s1), "s1");
    klee_make_symbolic(&s2, sizeof(s2), "s2");
    klee_make_symbolic(&s3, sizeof(s3), "s3");
    klee_make_symbolic(pad, sizeof(pad), "pad");

    int r = 0;

    // Layer 1: 4 unique functions
    switch (s1 >> 6) {
        case 0: r = L1_a(s1); break;
        case 1: r = L1_b(s1); break;
        case 2: r = L1_c(s1); break;
        case 3: r = L1_d(s1); break;
    }

    // State explosion between layers
    r = interleave(r, pad[0]);
    r = interleave(r, pad[1]);

    // Layer 2: 4 more unique functions
    switch (s2 >> 6) {
        case 0: r = L2_a(s2); break;
        case 1: r = L2_b(s2); break;
        case 2: r = L2_c(s2); break;
        case 3: r = L2_d(s2); break;
    }

    // State explosion between layers
    r = interleave(r, pad[2]);
    r = interleave(r, pad[3]);

    // Layer 3: 4 more unique functions
    switch (s3 >> 6) {
        case 0: r = L3_a(s3); break;
        case 1: r = L3_b(s3); break;
        case 2: r = L3_c(s3); break;
        case 3: r = L3_d(s3); break;
    }

    return r;
}
