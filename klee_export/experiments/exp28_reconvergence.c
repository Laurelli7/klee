// exp28: "Exponential reconvergence" — Two phases: Phase 1 creates
// 2^N states through branching. Phase 2 is a SINGLE check that
// all states must pass through. Tests whether searchers get
// stuck multiplying states in Phase 1 or efficiently reach Phase 2.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int final_check(int x) {
    if (x == 12345) return -1; // BUG
    return x;
}

int main() {
    uint8_t data[5];
    klee_make_symbolic(data, sizeof(data), "data");

    // Phase 1: Branching explosion (5 bytes × 5 bits = 2^25 states)
    int acc = 0;
    for (int i = 0; i < 5; i++) {
        if (data[i] & 0x01) acc += (1 << (i*5));
        if (data[i] & 0x02) acc += (1 << (i*5+1));
        if (data[i] & 0x04) acc += (1 << (i*5+2));
        if (data[i] & 0x08) acc += (1 << (i*5+3));
        if (data[i] & 0x10) acc += (1 << (i*5+4));
    }

    // Phase 2: All states reconverge through this call
    return final_check(acc);
}
