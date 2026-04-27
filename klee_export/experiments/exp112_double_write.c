// exp112: "Double Symbolic Write" — Direct refinement of exp73.
// Same structure but with parameters tuned to amplify BFS advantage:
// - 6 writes to an 8-element table (high collision rate)  
// - 4 reads (more branching after writes)
// - Each read has unique handlers (more coverage to differentiate)
//
// Key idea: more writes + smaller table = more collisions = more
// constraint interactions. More reads = more chances for different
// collision states to produce different code paths.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int hit_1(int x) { return x + 0x11; }
__attribute__((noinline)) int miss_1(int x) { return x + 0x12; }
__attribute__((noinline)) int hit_2(int x) { return x + 0x21; }
__attribute__((noinline)) int miss_2(int x) { return x + 0x22; }
__attribute__((noinline)) int hit_3(int x) { return x + 0x31; }
__attribute__((noinline)) int miss_3(int x) { return x + 0x32; }
__attribute__((noinline)) int hit_4(int x) { return x + 0x41; }
__attribute__((noinline)) int miss_4(int x) { return x + 0x42; }

int main() {
    uint8_t data[10]; // 6 write indices + 4 read indices
    klee_make_symbolic(data, sizeof(data), "data");

    int table[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    // 6 symbolic writes — each marks a slot
    table[data[0] & 0x07] = 1;
    table[data[1] & 0x07] = 2;
    table[data[2] & 0x07] = 3;
    table[data[3] & 0x07] = 4;
    table[data[4] & 0x07] = 5;
    table[data[5] & 0x07] = 6;

    int r = 0;

    // 4 symbolic reads — branch on hit/miss
    int v1 = table[data[6] & 0x07];
    if (v1 != 0)
        r = hit_1(r);
    else
        r = miss_1(r);

    int v2 = table[data[7] & 0x07];
    if (v2 != 0)
        r = hit_2(r);
    else
        r = miss_2(r);

    int v3 = table[data[8] & 0x07];
    if (v3 != 0)
        r = hit_3(r);
    else
        r = miss_3(r);

    int v4 = table[data[9] & 0x07];
    if (v4 != 0)
        r = hit_4(r);
    else
        r = miss_4(r);

    return r;
}
