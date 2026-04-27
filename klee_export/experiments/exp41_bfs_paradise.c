// exp41: "BFS Paradise" — A program specifically designed to make BFS
// massively outperform DFS. Structure: 8 independent shallow sub-problems,
// each at depth 3 with a unique function. All 8 are siblings at the top
// level (no ordering dependency). DFS picks one and goes to depth 3,
// then backtracks; BFS discovers all 8 at depth 1 immediately and covers
// all unique functions in its first few scheduling rounds.
//
// Crucially, each sub-problem also has a DEEP tail (depth 20) after the
// unique function, so DFS gets trapped in the first sub-problem's tail.
//
// Expected: BFS >>> DFS
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int fn_A(int x) { return x * 2 + 1; }
__attribute__((noinline)) int fn_B(int x) { return x * 3 + 2; }
__attribute__((noinline)) int fn_C(int x) { return x * 5 + 3; }
__attribute__((noinline)) int fn_D(int x) { return x * 7 + 4; }
__attribute__((noinline)) int fn_E(int x) { return x * 11 + 5; }
__attribute__((noinline)) int fn_F(int x) { return x * 13 + 6; }
__attribute__((noinline)) int fn_G(int x) { return x * 17 + 7; }
__attribute__((noinline)) int fn_H(int x) { return x * 19 + 8; }

int main() {
    uint8_t selector;
    uint8_t tail[3]; // 3 bytes = 24 bits of tail per sub-problem
    klee_make_symbolic(&selector, sizeof(selector), "sel");
    klee_make_symbolic(tail, sizeof(tail), "tail");

    int r = 0;

    // 8 sub-problems based on top 3 bits of selector
    uint8_t which = selector >> 5; // 0..7

    switch (which) {
        case 0: r = fn_A(selector); break;
        case 1: r = fn_B(selector); break;
        case 2: r = fn_C(selector); break;
        case 3: r = fn_D(selector); break;
        case 4: r = fn_E(selector); break;
        case 5: r = fn_F(selector); break;
        case 6: r = fn_G(selector); break;
        case 7: r = fn_H(selector); break;
    }

    // DEEP TAIL: 24 independent branches (identical for all sub-problems)
    // DFS enters here immediately after first sub-problem and gets stuck
    for (int i = 0; i < 3; i++) {
        if (tail[i] & 0x01) r++;
        if (tail[i] & 0x02) r++;
        if (tail[i] & 0x04) r++;
        if (tail[i] & 0x08) r++;
        if (tail[i] & 0x10) r++;
        if (tail[i] & 0x20) r++;
        if (tail[i] & 0x40) r++;
        if (tail[i] & 0x80) r++;
    }

    return r;
}
