// exp111: "Write-Read Interleave" — Alternating symbolic writes and
// reads on a shared small table. Each write changes the table,
// each read branches on the current slot value.
//
// The read at depth N sees a table shaped by all prior writes.
// BFS ensures maximum diversity of table states at each depth.
// DFS commits to one write sequence. covnew sees all reads as
// "similar" coverage and can't efficiently plan the interleaving.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int on_empty(int x, int id) { return x + id; }
__attribute__((noinline)) int on_val1(int x, int id)  { return x + id + 0x100; }
__attribute__((noinline)) int on_val2(int x, int id)  { return x + id + 0x200; }
__attribute__((noinline)) int on_val3(int x, int id)  { return x + id + 0x300; }
__attribute__((noinline)) int on_val4(int x, int id)  { return x + id + 0x400; }
__attribute__((noinline)) int on_over(int x, int id)  { return x + id + 0x500; }

int main() {
    uint8_t ops[6]; // 6 symbolic operation bytes
    klee_make_symbolic(ops, sizeof(ops), "ops");

    int table[4] = {0, 0, 0, 0};
    int r = 0;

    // Round 1: Write
    int wi1 = ops[0] & 0x03;
    table[wi1] = 1;

    // Round 2: Read + branch
    int ri1 = ops[1] & 0x03;
    int v1 = table[ri1];
    switch (v1) {
        case 0: r = on_empty(r, 1); break;
        case 1: r = on_val1(r, 1); break;
        default: r = on_over(r, 1); break;
    }

    // Round 3: Write (accumulate)
    int wi2 = ops[2] & 0x03;
    table[wi2] += 2;

    // Round 4: Read + branch
    int ri2 = ops[3] & 0x03;
    int v2 = table[ri2];
    switch (v2) {
        case 0: r = on_empty(r, 2); break;
        case 1: r = on_val1(r, 2); break;
        case 2: r = on_val2(r, 2); break;
        case 3: r = on_val3(r, 2); break;
        default: r = on_over(r, 2); break;
    }

    // Round 5: Write
    int wi3 = ops[4] & 0x03;
    table[wi3] += 3;

    // Round 6: Final read
    int ri3 = ops[5] & 0x03;
    int v3 = table[ri3];
    switch (v3) {
        case 0: r = on_empty(r, 3); break;
        case 1: r = on_val1(r, 3); break;
        case 2: r = on_val2(r, 3); break;
        case 3: r = on_val3(r, 3); break;
        case 4: r = on_val4(r, 3); break;
        default: r = on_over(r, 3); break;
    }

    return r;
}
