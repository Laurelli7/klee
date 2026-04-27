// exp109: "Symbolic Index Cascade" — A chain of symbolic array reads
// where each read's result determines the next index. Creates
// exponential path diversity that BFS explores level-by-level.
//
// BFS explores all 8 possible values of table[idx[0]] before following
// any of them deeper, creating maximum first-level diversity.
// DFS follows one chain to completion. covnew/md2u chase nearby
// coverage but can't efficiently plan the cascade.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int handle_A(int x) { return x ^ 0xAA; }
__attribute__((noinline)) int handle_B(int x) { return x ^ 0xBB; }
__attribute__((noinline)) int handle_C(int x) { return x ^ 0xCC; }
__attribute__((noinline)) int handle_D(int x) { return x ^ 0xDD; }
__attribute__((noinline)) int handle_E(int x) { return x ^ 0xEE; }
__attribute__((noinline)) int handle_F(int x) { return x ^ 0xFF; }
__attribute__((noinline)) int handle_G(int x) { return x ^ 0x11; }
__attribute__((noinline)) int handle_H(int x) { return x ^ 0x22; }

int main() {
    uint8_t idx[3]; // 3 symbolic indices
    klee_make_symbolic(idx, sizeof(idx), "idx");

    // Fixed lookup table — deterministic content
    int table[8] = {3, 7, 1, 5, 0, 6, 2, 4};

    int r = 0;

    // Level 1: symbolic index into table
    int v1 = table[idx[0] & 0x07];

    // Branch on v1
    switch (v1) {
        case 0: r = handle_A(r); break;
        case 1: r = handle_B(r); break;
        case 2: r = handle_C(r); break;
        case 3: r = handle_D(r); break;
        case 4: r = handle_E(r); break;
        case 5: r = handle_F(r); break;
        case 6: r = handle_G(r); break;
        case 7: r = handle_H(r); break;
    }

    // Level 2: cascade — use v1 XOR idx[1] as next index
    int v2 = table[(v1 ^ idx[1]) & 0x07];

    switch (v2) {
        case 0: r = handle_A(r); break;
        case 1: r = handle_B(r); break;
        case 2: r = handle_C(r); break;
        case 3: r = handle_D(r); break;
        case 4: r = handle_E(r); break;
        case 5: r = handle_F(r); break;
        case 6: r = handle_G(r); break;
        case 7: r = handle_H(r); break;
    }

    // Level 3: cascade — v2 XOR idx[2]
    int v3 = table[(v2 ^ idx[2]) & 0x07];

    switch (v3) {
        case 0: r = handle_A(r); break;
        case 1: r = handle_B(r); break;
        case 2: r = handle_C(r); break;
        case 3: r = handle_D(r); break;
        case 4: r = handle_E(r); break;
        case 5: r = handle_F(r); break;
        case 6: r = handle_G(r); break;
        case 7: r = handle_H(r); break;
    }

    return r;
}
