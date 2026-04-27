// exp119: exp73 variant — 4 writes to 16-element table, 2 reads.
// SAME as exp73 but with UNIQUE collision handlers per write index.
// Write 0 collision and write 2 collision call different functions.
// This creates more unique code paths per collision pattern.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int coll_0(int x) { return x + 0xC0; }
__attribute__((noinline)) int coll_1(int x) { return x + 0xC1; }
__attribute__((noinline)) int coll_2(int x) { return x + 0xC2; }
__attribute__((noinline)) int coll_3(int x) { return x + 0xC3; }
__attribute__((noinline)) int fresh_0(int x) { return x + 0xF0; }
__attribute__((noinline)) int fresh_1(int x) { return x + 0xF1; }
__attribute__((noinline)) int fresh_2(int x) { return x + 0xF2; }
__attribute__((noinline)) int fresh_3(int x) { return x + 0xF3; }
__attribute__((noinline)) int hit_r0(int x) { return x + 0xA0; }
__attribute__((noinline)) int miss_r0(int x) { return x + 0xB0; }
__attribute__((noinline)) int hit_r1(int x) { return x + 0xA1; }
__attribute__((noinline)) int miss_r1(int x) { return x + 0xB1; }

int main() {
    uint8_t widx[4], wval[4], ridx[2];
    klee_make_symbolic(widx, sizeof(widx), "widx");
    klee_make_symbolic(wval, sizeof(wval), "wval");
    klee_make_symbolic(ridx, sizeof(ridx), "ridx");

    int table[16] = {0};
    int result = 0;

    // Write 0
    {
        uint8_t idx = widx[0] & 0x0F;
        if (table[idx] != 0) result = coll_0(result);
        else result = fresh_0(result);
        table[idx] = wval[0] + 1;
    }
    // Write 1
    {
        uint8_t idx = widx[1] & 0x0F;
        if (table[idx] != 0) result = coll_1(result);
        else result = fresh_1(result);
        table[idx] = wval[1] + 1;
    }
    // Write 2
    {
        uint8_t idx = widx[2] & 0x0F;
        if (table[idx] != 0) result = coll_2(result);
        else result = fresh_2(result);
        table[idx] = wval[2] + 1;
    }
    // Write 3
    {
        uint8_t idx = widx[3] & 0x0F;
        if (table[idx] != 0) result = coll_3(result);
        else result = fresh_3(result);
        table[idx] = wval[3] + 1;
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

    return result;
}
