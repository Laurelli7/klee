// exp61: "Scaled Pointer Chase" — exp57 amplified.
// 6 steps through 16 nodes (16^6 = 16.7M theoretical paths).
// This should create massive differentiation since exp57 was
// the best differentiator in wave 7.
//
// Key change from exp57: 16 nodes instead of 8, 6 steps instead of 5.
// With 16 targets per step, the fanout is enormous.
// Additionally, some nodes have "expensive" side effects (nested branches)
// and others are cheap. This should split NURS:qc from NURS:covnew.
//
// Expected: random-path/default best (tree sampling), DFS medium,
// BFS/covnew worst (state explosion or coverage chasing).
#include "klee/klee.h"
#include <stdint.h>

// 16 node handlers: first 8 cheap, next 8 have extra branching
__attribute__((noinline)) int node_00(int x) { return x + 100; }
__attribute__((noinline)) int node_01(int x) { return x + 101; }
__attribute__((noinline)) int node_02(int x) { return x + 102; }
__attribute__((noinline)) int node_03(int x) { return x + 103; }
__attribute__((noinline)) int node_04(int x) { return x + 104; }
__attribute__((noinline)) int node_05(int x) { return x + 105; }
__attribute__((noinline)) int node_06(int x) { return x + 106; }
__attribute__((noinline)) int node_07(int x) { return x + 107; }

// Expensive nodes: have conditional branches inside
__attribute__((noinline)) int node_08(int x) {
    if (x & 0x01) x += 200;
    if (x & 0x02) x += 201;
    return x;
}
__attribute__((noinline)) int node_09(int x) {
    if (x & 0x04) x += 300;
    if (x & 0x08) x += 301;
    return x;
}
__attribute__((noinline)) int node_10(int x) {
    if (x & 0x10) x += 400;
    if (x & 0x20) x += 401;
    return x;
}
__attribute__((noinline)) int node_11(int x) {
    if (x & 0x40) x += 500;
    return x;
}
__attribute__((noinline)) int node_12(int x) {
    if (x > 1000) x += 600;
    return x;
}
__attribute__((noinline)) int node_13(int x) {
    if (x < -1000) x += 700;
    return x;
}
__attribute__((noinline)) int node_14(int x) {
    if (x == 0) x += 800;
    return x;
}
__attribute__((noinline)) int node_15(int x) {
    if (x & 0x80) x += 900;
    if (x > 500) x += 901;
    return x;
}

typedef int (*handler_t)(int);

int main() {
    uint8_t path[6]; // 6 steps
    klee_make_symbolic(path, sizeof(path), "path");

    handler_t table[16] = {
        node_00, node_01, node_02, node_03,
        node_04, node_05, node_06, node_07,
        node_08, node_09, node_10, node_11,
        node_12, node_13, node_14, node_15
    };

    int result = 0;
    for (int step = 0; step < 6; step++) {
        uint8_t idx = path[step] & 0x0F; // 16 nodes
        result = table[idx](result);
    }

    // Classify final result
    if (result > 5000) return 1;
    if (result < -5000) return 2;
    if (result == 0) return 3;
    return 0;
}
