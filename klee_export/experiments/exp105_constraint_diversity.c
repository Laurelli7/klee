// exp105: "Constraint Diversity" — Mimics exp73 closely but with
// parameters tuned for maximum BFS advantage based on wave 11 learnings.
//
// exp73 parameters: 4 writes to 16-element table, 2 reads → BFS won by 5
// exp98 parameters: 6 writes to 8-element table, 3 reads → BFS tied DFS
//
// Theory: smaller table (more collisions) + fewer writes = more constraint
// diversity per level. Let's try 3 writes to 4-element table + 2 reads.
// This maximizes collision probability (3 writes, only 4 slots = ~75%
// chance of collision) while keeping the state space manageable.
//
// Also: add a leading 4-way switch to create sub-problems (anti-DFS).
#include "klee/klee.h"
#include <stdint.h>

// Per-subproblem unique handlers
__attribute__((noinline)) int hit_A(int x)  { return x + 0xA0; }
__attribute__((noinline)) int miss_A(int x) { return x + 0xA1; }
__attribute__((noinline)) int coll_A(int x) { return x + 0xA2; }

__attribute__((noinline)) int hit_B(int x)  { return x + 0xB0; }
__attribute__((noinline)) int miss_B(int x) { return x + 0xB1; }
__attribute__((noinline)) int coll_B(int x) { return x + 0xB2; }

__attribute__((noinline)) int hit_C(int x)  { return x + 0xC0; }
__attribute__((noinline)) int miss_C(int x) { return x + 0xC1; }
__attribute__((noinline)) int coll_C(int x) { return x + 0xC2; }

__attribute__((noinline)) int hit_D(int x)  { return x + 0xD0; }
__attribute__((noinline)) int miss_D(int x) { return x + 0xD1; }
__attribute__((noinline)) int coll_D(int x) { return x + 0xD2; }

typedef int (*handler_t)(int);

int main() {
    uint8_t sel;
    uint8_t widx[3], wval[3], ridx[2];
    klee_make_symbolic(&sel, sizeof(sel), "sel");
    klee_make_symbolic(widx, sizeof(widx), "widx");
    klee_make_symbolic(wval, sizeof(wval), "wval");
    klee_make_symbolic(ridx, sizeof(ridx), "ridx");

    int r = 0;
    int table[4] = {0};
    uint8_t sp = sel >> 6; // 0..3

    // 3 writes to 4-element table
    for (int i = 0; i < 3; i++) {
        uint8_t idx = widx[i] & 0x03;
        if (table[idx] != 0) {
            switch (sp) {
                case 0: r = coll_A(r); break;
                case 1: r = coll_B(r); break;
                case 2: r = coll_C(r); break;
                case 3: r = coll_D(r); break;
            }
        }
        table[idx] = wval[i] + 1;
    }

    // 2 reads
    for (int i = 0; i < 2; i++) {
        uint8_t idx = ridx[i] & 0x03;
        if (table[idx] != 0) {
            switch (sp) {
                case 0: r = hit_A(r); break;
                case 1: r = hit_B(r); break;
                case 2: r = hit_C(r); break;
                case 3: r = hit_D(r); break;
            }
        } else {
            switch (sp) {
                case 0: r = miss_A(r); break;
                case 1: r = miss_B(r); break;
                case 2: r = miss_C(r); break;
                case 3: r = miss_D(r); break;
            }
        }
    }

    return r;
}
