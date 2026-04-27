// exp123: exp73 variant — 4 writes to 16-element table, 2 reads.
// EXACT SAME as exp73 but WITHOUT write_val (deterministic values)
// and with UNIQUE handlers per collision step AND per read.
// This is the maximum-handler-diversity version.
#include "klee/klee.h"
#include <stdint.h>

// Collision handlers per write step
__attribute__((noinline)) int coll_w1(int x) { return x + 0x5001; }
__attribute__((noinline)) int coll_w2(int x) { return x + 0x5002; }
__attribute__((noinline)) int coll_w3(int x) { return x + 0x5003; }
// Fresh-write handlers per step
__attribute__((noinline)) int fresh_w0(int x) { return x + 0xF000; }
__attribute__((noinline)) int fresh_w1(int x) { return x + 0xF001; }
__attribute__((noinline)) int fresh_w2(int x) { return x + 0xF002; }
__attribute__((noinline)) int fresh_w3(int x) { return x + 0xF003; }
// Read handlers
__attribute__((noinline)) int hit_r0(int x) { return x + 0xA0; }
__attribute__((noinline)) int miss_r0(int x) { return x + 0xB0; }
__attribute__((noinline)) int hit_r1(int x) { return x + 0xA1; }
__attribute__((noinline)) int miss_r1(int x) { return x + 0xB1; }
// Full check
__attribute__((noinline)) int all_unique(int x) { return x + 0x9000; }

int main() {
    uint8_t widx[4], ridx[2];
    klee_make_symbolic(widx, sizeof(widx), "widx");
    klee_make_symbolic(ridx, sizeof(ridx), "ridx");

    int table[16] = {0};
    int result = 0;

    // Write 0 — always fresh (table is empty)
    {
        uint8_t idx = widx[0] & 0x0F;
        result = fresh_w0(result);
        table[idx] = 1;
    }
    // Write 1
    {
        uint8_t idx = widx[1] & 0x0F;
        if (table[idx] != 0) result = coll_w1(result);
        else result = fresh_w1(result);
        table[idx] = 2;
    }
    // Write 2
    {
        uint8_t idx = widx[2] & 0x0F;
        if (table[idx] != 0) result = coll_w2(result);
        else result = fresh_w2(result);
        table[idx] = 3;
    }
    // Write 3
    {
        uint8_t idx = widx[3] & 0x0F;
        if (table[idx] != 0) result = coll_w3(result);
        else result = fresh_w3(result);
        table[idx] = 4;
    }

    // Read 0
    {
        uint8_t idx = ridx[0] & 0x0F;
        if (table[idx] != 0) result = hit_r0(result);
        else result = miss_r0(result);
    }
    // Read 1
    {
        uint8_t idx = ridx[1] & 0x0F;
        if (table[idx] != 0) result = hit_r1(result);
        else result = miss_r1(result);
    }

    // Full check
    int count = 0;
    for (int i = 0; i < 16; i++)
        if (table[i] != 0) count++;
    if (count == 4)
        result = all_unique(result);

    return result;
}
