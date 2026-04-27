// exp116: exp73 variant — 3 writes to 16-element table, 4 reads.
// Fewer writes (shallower collision tree, faster to explore) but
// more reads (more coverage-relevant branches). The reads should
// differentiate searchers more than writes.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int handle_hit_0(int x) { return x + 0x100; }
__attribute__((noinline)) int handle_miss_0(int x) { return x + 0x101; }
__attribute__((noinline)) int handle_hit_1(int x) { return x + 0x200; }
__attribute__((noinline)) int handle_miss_1(int x) { return x + 0x201; }
__attribute__((noinline)) int handle_hit_2(int x) { return x + 0x300; }
__attribute__((noinline)) int handle_miss_2(int x) { return x + 0x301; }
__attribute__((noinline)) int handle_hit_3(int x) { return x + 0x400; }
__attribute__((noinline)) int handle_miss_3(int x) { return x + 0x401; }
__attribute__((noinline)) int handle_collision(int x) { return x + 0x500; }

int main() {
    uint8_t widx[3], wval[3], ridx[4];
    klee_make_symbolic(widx, sizeof(widx), "widx");
    klee_make_symbolic(wval, sizeof(wval), "wval");
    klee_make_symbolic(ridx, sizeof(ridx), "ridx");

    int table[16] = {0};
    int result = 0;

    for (int i = 0; i < 3; i++) {
        uint8_t idx = widx[i] & 0x0F;
        if (table[idx] != 0)
            result = handle_collision(result);
        table[idx] = wval[i] + 1;
    }

    uint8_t r0 = ridx[0] & 0x0F;
    if (table[r0] != 0) result = handle_hit_0(result);
    else result = handle_miss_0(result);

    uint8_t r1 = ridx[1] & 0x0F;
    if (table[r1] != 0) result = handle_hit_1(result);
    else result = handle_miss_1(result);

    uint8_t r2 = ridx[2] & 0x0F;
    if (table[r2] != 0) result = handle_hit_2(result);
    else result = handle_miss_2(result);

    uint8_t r3 = ridx[3] & 0x0F;
    if (table[r3] != 0) result = handle_hit_3(result);
    else result = handle_miss_3(result);

    return result;
}
