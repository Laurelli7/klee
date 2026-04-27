// exp38: "Mixed depth regions" — 4 regions at very different depths.
// Tests how well searchers balance breadth vs depth.
// Region A: depth 4 (shallow)
// Region B: depth 12 (medium)
// Region C: depth 24 (deep)
// Region D: depth 2 (trivial) with a bug
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int fn_A(int x) { return x + 10; }
__attribute__((noinline)) int fn_B(int x) { return x + 20; }
__attribute__((noinline)) int fn_C(int x) { return x + 30; }

int main() {
    uint8_t sel;
    uint8_t data[6];
    klee_make_symbolic(&sel, sizeof(sel), "sel");
    klee_make_symbolic(data, sizeof(data), "data");

    int r = 0;

    if (sel < 64) {
        // Region A: 2 bytes × 2 bits = depth 4
        if (data[0] & 0x01) r++;
        if (data[0] & 0x02) r++;
        if (data[1] & 0x01) r++;
        if (data[1] & 0x02) r++;
        r = fn_A(r);
    } else if (sel < 128) {
        // Region B: 2 bytes × 6 bits = depth 12
        for (int i = 0; i < 2; i++) {
            if (data[i] & 0x01) r++;
            if (data[i] & 0x02) r++;
            if (data[i] & 0x04) r++;
            if (data[i] & 0x08) r++;
            if (data[i] & 0x10) r++;
            if (data[i] & 0x20) r++;
        }
        r = fn_B(r);
    } else if (sel < 192) {
        // Region C: 4 bytes × 6 bits = depth 24 → 2^24 paths!
        for (int i = 0; i < 4; i++) {
            if (data[i] & 0x01) r++;
            if (data[i] & 0x02) r++;
            if (data[i] & 0x04) r++;
            if (data[i] & 0x08) r++;
            if (data[i] & 0x10) r++;
            if (data[i] & 0x20) r++;
        }
        r = fn_C(r);
    } else {
        // Region D: Trivial depth 2, contains bug
        if (data[0] == 0xBE && data[1] == 0xEF) {
            return -1; // BUG
        }
        return 0;
    }
    return r;
}
