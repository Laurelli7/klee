// exp104: "Write-Then-Dispatch" — Symbolic writes to a small table,
// then the TABLE CONTENTS determine which unique function to call.
// This creates a coverage pattern that DEPENDS on constraint relationships
// rather than explicit control flow.
//
// Key insight from exp73: BFS wins when code paths depend on symbolic
// constraint interactions. Here, what function gets called depends on
// what was written to the table — which depends on symbolic write indices
// and values. BFS's systematic level-by-level exploration creates more
// diverse constraint combinations than NURS heuristics.
//
// No deep tail — the constraint-dependent dispatch IS the differentiator.
// But we add enough symbolic variables that the state space is too large
// to complete in 10s, forcing searcher strategy to matter.
//
// Expected: BFS creates more diverse constraint combos → more coverage
#include "klee/klee.h"
#include <stdint.h>

// Dispatch handlers based on table contents
__attribute__((noinline)) int dispatch_0(int x) { return x + 10; }
__attribute__((noinline)) int dispatch_1(int x) { return x + 20; }
__attribute__((noinline)) int dispatch_2(int x) { return x + 30; }
__attribute__((noinline)) int dispatch_3(int x) { return x + 40; }
__attribute__((noinline)) int dispatch_4(int x) { return x + 50; }
__attribute__((noinline)) int dispatch_5(int x) { return x + 60; }
__attribute__((noinline)) int dispatch_6(int x) { return x + 70; }
__attribute__((noinline)) int dispatch_7(int x) { return x + 80; }

// Collision / empty handlers
__attribute__((noinline)) int on_write_empty(int x, int slot) { return x + slot * 7; }
__attribute__((noinline)) int on_write_collision(int x, int old, int nw) { return x + old * 3 + nw; }

int main() {
    uint8_t widx[4];  // 4 write indices
    uint8_t wval[4];  // 4 write values
    uint8_t dispatch_sel; // which cell to dispatch on
    klee_make_symbolic(widx, sizeof(widx), "widx");
    klee_make_symbolic(wval, sizeof(wval), "wval");
    klee_make_symbolic(&dispatch_sel, sizeof(dispatch_sel), "dsel");

    int table[8] = {0};
    int r = 0;

    // 4 symbolic writes to 8-element table
    for (int i = 0; i < 4; i++) {
        uint8_t idx = widx[i] & 0x07;
        int new_val = (wval[i] & 0x07) + 1; // values 1..8
        if (table[idx] == 0) {
            r = on_write_empty(r, idx);
            table[idx] = new_val;
        } else {
            r = on_write_collision(r, table[idx], new_val);
            table[idx] = new_val;
        }
    }

    // Dispatch on table contents — the function called depends on
    // which constraint combination was explored during writes
    uint8_t di = dispatch_sel & 0x07;
    int cell = table[di];
    switch (cell) {
        case 0: r = dispatch_0(r); break;
        case 1: r = dispatch_1(r); break;
        case 2: r = dispatch_2(r); break;
        case 3: r = dispatch_3(r); break;
        case 4: r = dispatch_4(r); break;
        case 5: r = dispatch_5(r); break;
        case 6: r = dispatch_6(r); break;
        case 7: r = dispatch_7(r); break;
        default: r = dispatch_0(r); break; // 8 maps to 0
    }

    return r;
}
