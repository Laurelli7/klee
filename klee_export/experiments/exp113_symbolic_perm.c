// exp113: "Symbolic Permutation" — Exploits the fact that BFS creates
// more diverse constraint sets at each depth level.
//
// Given 4 symbolic bytes, we use them to index into a small permutation
// table. The permutation output determines which of 4 unique paths to
// take. Because the value depends on the full symbolic index, BFS
// spreads exploration across all possible index values at each step,
// maximizing the variety of permutation outputs seen.
//
// The twist: after the permutation lookups, we combine results and 
// branch into one of many handler functions based on the combination.
// BFS has seen the most combinations → covers the most handlers.
#include "klee/klee.h"
#include <stdint.h>

// 16 unique handlers for permutation combinations
__attribute__((noinline)) int h00(int x) { return x + 0x00; }
__attribute__((noinline)) int h01(int x) { return x + 0x01; }
__attribute__((noinline)) int h02(int x) { return x + 0x02; }
__attribute__((noinline)) int h03(int x) { return x + 0x03; }
__attribute__((noinline)) int h10(int x) { return x + 0x10; }
__attribute__((noinline)) int h11(int x) { return x + 0x11; }
__attribute__((noinline)) int h12(int x) { return x + 0x12; }
__attribute__((noinline)) int h13(int x) { return x + 0x13; }
__attribute__((noinline)) int h20(int x) { return x + 0x20; }
__attribute__((noinline)) int h21(int x) { return x + 0x21; }
__attribute__((noinline)) int h22(int x) { return x + 0x22; }
__attribute__((noinline)) int h23(int x) { return x + 0x23; }
__attribute__((noinline)) int h30(int x) { return x + 0x30; }
__attribute__((noinline)) int h31(int x) { return x + 0x31; }
__attribute__((noinline)) int h32(int x) { return x + 0x32; }
__attribute__((noinline)) int h33(int x) { return x + 0x33; }

int main() {
    uint8_t inp[2]; // 2 symbolic bytes
    klee_make_symbolic(inp, sizeof(inp), "inp");

    // Two independent permutation tables
    int perm_a[8] = {2, 0, 3, 1, 3, 2, 0, 1};
    int perm_b[8] = {1, 3, 0, 2, 0, 1, 3, 2};

    // Look up permutation values
    int a = perm_a[inp[0] & 0x07]; // 0-3
    int b = perm_b[inp[1] & 0x07]; // 0-3

    // Combine: 4 × 4 = 16 possible combinations
    int combo = a * 4 + b;

    int r = 0;
    switch (combo) {
        case 0:  r = h00(r); break;
        case 1:  r = h01(r); break;
        case 2:  r = h02(r); break;
        case 3:  r = h03(r); break;
        case 4:  r = h10(r); break;
        case 5:  r = h11(r); break;
        case 6:  r = h12(r); break;
        case 7:  r = h13(r); break;
        case 8:  r = h20(r); break;
        case 9:  r = h21(r); break;
        case 10: r = h22(r); break;
        case 11: r = h23(r); break;
        case 12: r = h30(r); break;
        case 13: r = h31(r); break;
        case 14: r = h32(r); break;
        case 15: r = h33(r); break;
    }

    return r;
}
