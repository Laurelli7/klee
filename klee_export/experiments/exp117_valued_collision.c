// exp117: exp73 variant — writes with VALUE-DEPENDENT collision handling.
// Instead of all collisions calling the same handler, different collision
// patterns call different handlers. This means the coverage difference
// between searchers depends on WHICH collision patterns they explore.
//
// The key change: we distinguish between first-time collision,
// double collision, and triple collision at the same slot.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int first_write(int x) { return x + 0x10; }
__attribute__((noinline)) int single_coll(int x) { return x + 0x20; }
__attribute__((noinline)) int double_coll(int x) { return x + 0x40; }
__attribute__((noinline)) int triple_coll(int x) { return x + 0x80; }
__attribute__((noinline)) int hit_read(int x, int v) { return x + v * 7; }
__attribute__((noinline)) int miss_read(int x) { return x + 3; }

int main() {
    uint8_t widx[4], ridx[2];
    klee_make_symbolic(widx, sizeof(widx), "widx");
    klee_make_symbolic(ridx, sizeof(ridx), "ridx");

    // Table stores WRITE COUNT per slot (not just occupied/not)
    int table[16] = {0};
    int result = 0;

    for (int i = 0; i < 4; i++) {
        uint8_t idx = widx[i] & 0x0F;
        int prev = table[idx];
        if (prev == 0)
            result = first_write(result);
        else if (prev == 1)
            result = single_coll(result);
        else if (prev == 2)
            result = double_coll(result);
        else
            result = triple_coll(result);
        table[idx] = prev + 1;
    }

    for (int i = 0; i < 2; i++) {
        uint8_t idx = ridx[i] & 0x0F;
        int val = table[idx];
        if (val != 0)
            result = hit_read(result, val);
        else
            result = miss_read(result);
    }

    return result;
}
