// exp35: "Constraint tightening" — Each successive branch TIGHTENS
// constraints rather than splitting. e.g. x>100, then x>150, then x>200.
// The solver sees incremental refinements. DFS walks down a single
// tightening chain, others spread across ranges.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int reward_A(int v) { return v + 1111; }
__attribute__((noinline)) int reward_B(int v) { return v + 2222; }
__attribute__((noinline)) int reward_C(int v) { return v + 3333; }
__attribute__((noinline)) int reward_D(int v) { return v + 4444; }
__attribute__((noinline)) int reward_E(int v) { return v + 5555; }

int main() {
    uint32_t x;
    uint8_t noise[4];
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int r = 0;
    // Noise wall to create paths
    for (int i = 0; i < 4; i++) {
        if (noise[i] & 0x01) r++;
        if (noise[i] & 0x02) r++;
        if (noise[i] & 0x04) r++;
    }

    // Tightening chain: each gate requires progressively stricter x
    if (x > 100) {
        r = reward_A(r);
        if (x > 1000) {
            r = reward_B(r);
            if (x > 10000) {
                r = reward_C(r);
                if (x > 100000) {
                    r = reward_D(r);
                    if (x > 1000000) {
                        r = reward_E(r);
                    }
                }
            }
        }
    }
    return r;
}
