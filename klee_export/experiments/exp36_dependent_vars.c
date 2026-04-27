// exp36: "Dependent symbolic variables" — y depends on x through
// a series of transformations. Creates non-trivial constraint
// relationships between variables.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int special_A(int v) { return v * 2; }
__attribute__((noinline)) int special_B(int v) { return v * 3; }

int main() {
    uint8_t x, y, z;
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(&y, sizeof(y), "y");
    klee_make_symbolic(&z, sizeof(z), "z");

    int r = 0;

    // Chain of dependent comparisons
    if (x > y) {
        r += 1;
        if (y > z) {
            r += 2;
            // x > y > z — transitivity chain
            if (x - z > 100) {
                r = special_A(r);
            }
        } else {
            r += 4;
            // x > y, y <= z
            if (z - x > 50) {
                r = special_B(r);
            }
        }
    } else {
        r += 8;
        if (y > z) {
            r += 16;
            if (y - z > 100) {
                r += 32;
            }
        } else {
            // x <= y <= z
            r += 64;
            if (z - x > 200) {
                r += 128;
            }
        }
    }

    // Cross-variable check
    if ((x ^ y ^ z) == 0x42) return -1; // BUG
    return r;
}
