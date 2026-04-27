// exp120: exp73 variant — 4 writes to 32-element table, 2 reads.
// Larger table → fewer collisions → more write-combination diversity.
// 5-bit index mask → 32 possible values per write → 32^4 ≈ 1M write
// combinations. Should be too large to complete.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int handle_hit(int x) { return x + 1000; }
__attribute__((noinline)) int handle_miss(int x) { return x + 1; }
__attribute__((noinline)) int handle_collision(int x) { return x + 5000; }
__attribute__((noinline)) int handle_full(int x) { return x + 9000; }

int main() {
    uint8_t widx[4], wval[4], ridx[2];
    klee_make_symbolic(widx, sizeof(widx), "widx");
    klee_make_symbolic(wval, sizeof(wval), "wval");
    klee_make_symbolic(ridx, sizeof(ridx), "ridx");

    int table[32] = {0};
    int result = 0;

    for (int i = 0; i < 4; i++) {
        uint8_t idx = widx[i] & 0x1F;  // 32 positions
        if (table[idx] != 0)
            result = handle_collision(result);
        table[idx] = wval[i] + 1;
    }

    for (int i = 0; i < 2; i++) {
        uint8_t idx = ridx[i] & 0x1F;
        if (table[idx] != 0)
            result = handle_hit(result);
        else
            result = handle_miss(result);
    }

    int count = 0;
    for (int i = 0; i < 32; i++)
        if (table[i] != 0) count++;
    if (count == 4)
        result = handle_full(result);

    return result;
}
