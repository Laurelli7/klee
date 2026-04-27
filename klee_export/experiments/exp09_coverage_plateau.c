// exp09: "Misguided coverage" — many branches have identical coverage
// implications (they all cover the same code), but searchers that
// prioritize coverage novelty will thrash between them.
#include "klee/klee.h"
int main() {
    unsigned x, y;
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(&y, sizeof(y), "y");

    int v;
    // 32 branches from x's bits — all lead to same code
    v = __builtin_popcount(x);

    // After the popcount-equivalent expansion, one specific check
    if (v == 16 && y == 0xDEAD) {
        return -1; // BUG: needs exactly 16 bits set in x AND y==0xDEAD
    }

    if (v > 20) return 1;
    if (v > 10) return 2;
    return 0;
}
