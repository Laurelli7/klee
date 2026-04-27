// exp34: "Time-to-first-coverage" — A program where different searchers
// reach different coverage milestones at different times.
// 3 independent regions, each behind a different gate.
// Region A: behind (x[0] == 'X') — shallow
// Region B: behind (x[1] == 'Y') — medium depth
// Region C: behind (x[2] == 'Z') — deep (behind bitfield wall)
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int region_A(int x) { return x + 100; }
__attribute__((noinline)) int region_B(int x) { return x + 200; }
__attribute__((noinline)) int region_C(int x) { return x + 300; }

int main() {
    uint8_t selector;
    uint8_t gate;
    uint8_t noise[5];
    klee_make_symbolic(&selector, sizeof(selector), "selector");
    klee_make_symbolic(&gate, sizeof(gate), "gate");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int r = 0;

    if (selector < 85) {
        // Region A: Shallow (2 branches)
        if (gate == 'X') r = region_A(gate);
        return r;
    } else if (selector < 170) {
        // Region B: Medium (2 branches + 10 bitfield branches)
        if (gate == 'Y') {
            for (int i = 0; i < 2; i++) {
                if (noise[i] & 0x01) r++;
                if (noise[i] & 0x02) r++;
                if (noise[i] & 0x04) r++;
                if (noise[i] & 0x08) r++;
                if (noise[i] & 0x10) r++;
            }
            r = region_B(r);
        }
        return r;
    } else {
        // Region C: Deep (2 branches + 25 bitfield branches)
        if (gate == 'Z') {
            for (int i = 0; i < 5; i++) {
                if (noise[i] & 0x01) r++;
                if (noise[i] & 0x02) r++;
                if (noise[i] & 0x04) r++;
                if (noise[i] & 0x08) r++;
                if (noise[i] & 0x10) r++;
            }
            r = region_C(r);
        }
        return r;
    }
}
