// exp22: "State count pressure" — Tests what happens when we hit
// max-memory limits. With --max-memory=100, the memory-constrained
// searcher should show different fork inhibition patterns.
#include "klee/klee.h"
#include <stdint.h>

int main() {
    uint8_t data[6];
    klee_make_symbolic(data, sizeof(data), "data");

    int r = 0;
    // 6 × 6 bits = 36 branches, 2^36 theoretical paths
    // With memory limits, searchers must be selective
    for (int i = 0; i < 6; i++) {
        if (data[i] & 0x01) r += (1 << (i*6 + 0));
        if (data[i] & 0x02) r += (1 << (i*6 + 1));
        if (data[i] & 0x04) r += (1 << (i*6 + 2));
        if (data[i] & 0x08) r += (1 << (i*6 + 3));
        if (data[i] & 0x10) r += (1 << (i*6 + 4));
        if (data[i] & 0x20) r += (1 << (i*6 + 5));
    }

    if (r == 0xDEAD) return -1;
    return r & 0xFF;
}
