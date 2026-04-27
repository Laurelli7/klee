// exp16: "Solver cost gradient" — Some paths create cheap constraints
// (simple equalities), others create expensive constraints (multiplication chains).
// NURS:qc should prefer the cheap paths and avoid the expensive ones.
#include "klee/klee.h"
#include <stdint.h>

int main() {
    uint32_t x, y;
    uint8_t selector;
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(&y, sizeof(y), "y");
    klee_make_symbolic(&selector, sizeof(selector), "selector");

    if (selector < 85) {
        // REGION A: Cheap constraints (simple comparisons)
        int score = 0;
        if (x > 100) score++;
        if (x > 200) score++;
        if (x > 500) score++;
        if (x > 1000) score++;
        if (x > 5000) score++;
        if (x > 10000) score++;
        if (y > 100) score++;
        if (y > 200) score++;
        if (y > 500) score++;
        if (y > 1000) score++;
        if (y > 5000) score++;
        if (y > 10000) score++;
        if (score == 12) return -1; // easy bug
        return score;
    } else if (selector < 170) {
        // REGION B: Medium constraints (multiplication)
        uint32_t prod = x * y;
        if (prod > 1000000) {
            if (prod < 2000000) {
                if ((prod % 7) == 3) {
                    return -2; // medium bug
                }
                return 1;
            }
            return 2;
        }
        return 3;
    } else {
        // REGION C: Expensive constraints (chain of multiplications)
        uint32_t h = x;
        h = h * 0x85ebca6b;
        h = h ^ (h >> 13);
        h = h * 0xc2b2ae35;
        h = h ^ (h >> 16);
        if (h == 0xdeadbeef) {
            return -3; // very hard bug
        }
        if ((h & 0xFF) == y) {
            return 4;
        }
        return 5;
    }
}
