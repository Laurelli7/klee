// exp19: "Solver cost war" — Path A has trivial constraints.
// Path B has expensive multiplication/modulo constraints.
// With --max-solver-time, NURS:qc should avoid Path B's timeouts.
// Without solver timeout, all searchers block equally.
#include "klee/klee.h"
#include <stdint.h>

int main() {
    uint32_t x, y;
    uint8_t selector;
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(&y, sizeof(y), "y");
    klee_make_symbolic(&selector, sizeof(selector), "selector");

    if (selector < 128) {
        // PATH A: 20 cheap branches
        int score = 0;
        for (int bit = 0; bit < 20; bit++) {
            if (x & (1u << bit)) score++;
        }
        if (score == 20) return -1;
        return score;
    } else {
        // PATH B: Expensive modular arithmetic chains
        uint32_t h = x * 0x9e3779b9;
        for (int i = 0; i < 10; i++) {
            if ((h % (i + 7)) == 0) {
                h = h * 0x85ebca6b + y;
            }
        }
        if (h == 0xdeadbeef) return -2;
        return (int)(h & 0xFF);
    }
}
