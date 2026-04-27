// exp26: "Heavy hash vs light paths" — 100-iteration hash to
// actually trigger solver timeouts. Previous attempts (30 iter)
// were too fast. With --max-solver-time=0.5s, the hash query
// should timeout and NURS:qc should learn to avoid it.
#include "klee/klee.h"
#include <stdint.h>

uint32_t heavy_hash(uint32_t x) {
    uint32_t h = x;
    for (int i = 0; i < 100; i++) {
        h = (h ^ (h >> 16)) * 0x85ebca6b;
        h = (h ^ (h >> 13)) * 0xc2b2ae35;
        h = h ^ (h >> 16);
    }
    return h;
}

int main() {
    uint32_t x;
    uint8_t data[3];
    uint8_t mode;
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(data, sizeof(data), "data");
    klee_make_symbolic(&mode, sizeof(mode), "mode");

    if (mode < 200) {
        // CHEAP path: 3 bytes × 3 bits = 9 branches, 512 paths
        int score = 0;
        for (int i = 0; i < 3; i++) {
            if (data[i] & 0x01) score++;
            if (data[i] & 0x02) score++;
            if (data[i] & 0x04) score++;
        }
        if (score == 9) return -1;
        return score;
    } else {
        // EXPENSIVE path: hash inversion
        if (heavy_hash(x) == 0xCAFEBABE) {
            return -2;
        }
        return 0;
    }
}
