// exp25: "Solver timeout divergence" — A program with an extremely
// expensive path (hash inversion) alongside many cheap paths.
// With --max-solver-time, NURS:qc should learn to avoid the
// expensive path while other searchers waste time on it.
#include "klee/klee.h"
#include <stdint.h>

uint32_t mini_hash(uint32_t x) {
    uint32_t h = x;
    for (int i = 0; i < 30; i++) {
        h = (h ^ (h >> 16)) * 0x85ebca6b;
        h = (h ^ (h >> 13)) * 0xc2b2ae35;
        h = h ^ (h >> 16);
    }
    return h;
}

int main() {
    uint32_t x;
    uint8_t mode;
    uint8_t data[3];
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(&mode, sizeof(mode), "mode");
    klee_make_symbolic(data, sizeof(data), "data");

    if (mode < 200) {
        // CHEAP: Simple bit-test paths
        int score = 0;
        for (int i = 0; i < 3; i++) {
            if (data[i] & 0x01) score++;
            if (data[i] & 0x02) score++;
            if (data[i] & 0x04) score++;
        }
        if (score == 9) return -1;
        return score;
    } else {
        // EXPENSIVE: Hash inversion (will timeout the solver)
        if (mini_hash(x) == 0xCAFEBABE) {
            return -2;
        }
        return 0;
    }
}
