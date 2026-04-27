// exp121: exp73 variant — remove write_val symbolic bytes.
// In exp73, write_val is symbolic but only used for the write value
// (not for branching). This adds constraint complexity without
// creating coverage-relevant branches. Removing it simplifies the
// solver's job and may let BFS cover more in 10s.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int handle_hit(int x) { return x + 1000; }
__attribute__((noinline)) int handle_miss(int x) { return x + 1; }
__attribute__((noinline)) int handle_collision(int x) { return x + 5000; }
__attribute__((noinline)) int handle_full(int x) { return x + 9000; }

int main() {
    uint8_t widx[4], ridx[2];
    klee_make_symbolic(widx, sizeof(widx), "widx");
    klee_make_symbolic(ridx, sizeof(ridx), "ridx");

    int table[16] = {0};
    int result = 0;

    for (int i = 0; i < 4; i++) {
        uint8_t idx = widx[i] & 0x0F;
        if (table[idx] != 0)
            result = handle_collision(result);
        table[idx] = i + 1;  // deterministic value
    }

    for (int i = 0; i < 2; i++) {
        uint8_t idx = ridx[i] & 0x0F;
        if (table[idx] != 0)
            result = handle_hit(result);
        else
            result = handle_miss(result);
    }

    int count = 0;
    for (int i = 0; i < 16; i++)
        if (table[i] != 0) count++;
    if (count == 4)
        result = handle_full(result);

    return result;
}
