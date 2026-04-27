// exp122: exp73 variant — 6 writes to 16-element table, 2 reads.
// More writes = deeper collision tree. With 6 writes to 16 slots,
// collisions are more likely, creating richer constraint interactions.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int handle_hit(int x) { return x + 1000; }
__attribute__((noinline)) int handle_miss(int x) { return x + 1; }
__attribute__((noinline)) int handle_collision(int x) { return x + 5000; }

int main() {
    uint8_t widx[6], wval[6], ridx[2];
    klee_make_symbolic(widx, sizeof(widx), "widx");
    klee_make_symbolic(wval, sizeof(wval), "wval");
    klee_make_symbolic(ridx, sizeof(ridx), "ridx");

    int table[16] = {0};
    int result = 0;

    for (int i = 0; i < 6; i++) {
        uint8_t idx = widx[i] & 0x0F;
        if (table[idx] != 0)
            result = handle_collision(result);
        table[idx] = wval[i] + 1;
    }

    for (int i = 0; i < 2; i++) {
        uint8_t idx = ridx[i] & 0x0F;
        if (table[idx] != 0)
            result = handle_hit(result);
        else
            result = handle_miss(result);
    }

    return result;
}
