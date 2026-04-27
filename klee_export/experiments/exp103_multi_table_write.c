// exp103: "Multi-Table Write" — Combines the switch-based anti-DFS pattern
// with symbolic writes that confuse covnew/md2u.
//
// 4 sub-problems selected by top 2 bits of selector. Each sub-problem
// has its own small table and does symbolic writes/reads with UNIQUE
// handler functions. After the sub-problem, a shared deep tail traps DFS.
//
// The symbolic writes create constraint interactions that make covnew/md2u
// heuristics unreliable (as demonstrated by exp73/exp98), while the
// switch+tail structure prevents DFS from exploring multiple sub-problems.
//
// Expected: BFS visits all 4 sub-problems at depth 1, explores write
// patterns across all sub-problems → highest coverage.
#include "klee/klee.h"
#include <stdint.h>

// Sub-problem 0 handlers
__attribute__((noinline)) int sp0_hit(int x)  { return x + 100; }
__attribute__((noinline)) int sp0_miss(int x) { return x + 200; }
__attribute__((noinline)) int sp0_coll(int x) { return x + 300; }

// Sub-problem 1 handlers
__attribute__((noinline)) int sp1_hit(int x)  { return x + 1100; }
__attribute__((noinline)) int sp1_miss(int x) { return x + 1200; }
__attribute__((noinline)) int sp1_coll(int x) { return x + 1300; }

// Sub-problem 2 handlers
__attribute__((noinline)) int sp2_hit(int x)  { return x + 2100; }
__attribute__((noinline)) int sp2_miss(int x) { return x + 2200; }
__attribute__((noinline)) int sp2_coll(int x) { return x + 2300; }

// Sub-problem 3 handlers
__attribute__((noinline)) int sp3_hit(int x)  { return x + 3100; }
__attribute__((noinline)) int sp3_miss(int x) { return x + 3200; }
__attribute__((noinline)) int sp3_coll(int x) { return x + 3300; }

int main() {
    uint8_t sel;
    uint8_t widx[3]; // 3 symbolic write indices
    uint8_t wval[3]; // 3 symbolic write values
    uint8_t ridx;    // 1 symbolic read index
    uint8_t tail[3]; // deep tail data
    klee_make_symbolic(&sel, sizeof(sel), "sel");
    klee_make_symbolic(widx, sizeof(widx), "widx");
    klee_make_symbolic(wval, sizeof(wval), "wval");
    klee_make_symbolic(&ridx, sizeof(ridx), "ridx");
    klee_make_symbolic(tail, sizeof(tail), "tail");

    int r = 0;
    int table[4] = {0};

    // Select sub-problem
    uint8_t sp = sel >> 6; // 0..3

    // Each sub-problem: 3 writes to a 4-element table, then 1 read
    for (int i = 0; i < 3; i++) {
        uint8_t idx = widx[i] & 0x03; // 4 positions
        if (table[idx] != 0) {
            switch (sp) {
                case 0: r = sp0_coll(r); break;
                case 1: r = sp1_coll(r); break;
                case 2: r = sp2_coll(r); break;
                case 3: r = sp3_coll(r); break;
            }
        }
        table[idx] = wval[i] + 1;
    }

    // Read
    uint8_t ri = ridx & 0x03;
    if (table[ri] != 0) {
        switch (sp) {
            case 0: r = sp0_hit(r); break;
            case 1: r = sp1_hit(r); break;
            case 2: r = sp2_hit(r); break;
            case 3: r = sp3_hit(r); break;
        }
    } else {
        switch (sp) {
            case 0: r = sp0_miss(r); break;
            case 1: r = sp1_miss(r); break;
            case 2: r = sp2_miss(r); break;
            case 3: r = sp3_miss(r); break;
        }
    }

    // Deep tail — shared, no new coverage, traps DFS
    for (int i = 0; i < 3; i++) {
        if (tail[i] & 0x01) r ^= 1;
        if (tail[i] & 0x02) r ^= 2;
        if (tail[i] & 0x04) r ^= 4;
        if (tail[i] & 0x08) r ^= 8;
        if (tail[i] & 0x10) r ^= 16;
        if (tail[i] & 0x20) r ^= 32;
        if (tail[i] & 0x40) r ^= 64;
        if (tail[i] & 0x80) r ^= 128;
    }

    return r;
}
