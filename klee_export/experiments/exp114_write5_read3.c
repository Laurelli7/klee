// exp114: exp73 variant — 5 writes to 16-element table, 3 reads.
// More writes = deeper collision tree, more reads = more code coverage
// to differentiate. Expect larger state space than exp73.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int handle_hit(int x, int id) { return x + 1000 + id; }
__attribute__((noinline)) int handle_miss(int x, int id) { return x + 1 + id; }
__attribute__((noinline)) int handle_collision(int x, int id) { return x + 5000 + id; }

int main() {
    uint8_t widx[5], wval[5], ridx[3];
    klee_make_symbolic(widx, sizeof(widx), "widx");
    klee_make_symbolic(wval, sizeof(wval), "wval");
    klee_make_symbolic(ridx, sizeof(ridx), "ridx");

    int table[16] = {0};
    int result = 0;

    for (int i = 0; i < 5; i++) {
        uint8_t idx = widx[i] & 0x0F;
        if (table[idx] != 0) {
            result = handle_collision(result, i);
        }
        table[idx] = wval[i] + 1;
    }

    for (int i = 0; i < 3; i++) {
        uint8_t idx = ridx[i] & 0x0F;
        if (table[idx] != 0) {
            result = handle_hit(result, i);
        } else {
            result = handle_miss(result, i);
        }
    }

    return result;
}
