// exp21: "Interleaved hot/cold" — Alternating between regions of
// already-covered "hot" code and new "cold" code.
// NURS:covnew should excel at finding cold spots efficiently.
// NURS:md2u should too, but via distance rather than coverage novelty.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int cold_fn_1(int x) { return x * 2 + 100; }
__attribute__((noinline)) int cold_fn_2(int x) { return x * 3 + 200; }
__attribute__((noinline)) int cold_fn_3(int x) { return x * 5 + 300; }
__attribute__((noinline)) int cold_fn_4(int x) { return x * 7 + 400; }

int main() {
    uint8_t a, b, c, d;
    uint8_t noise[4];
    klee_make_symbolic(&a, sizeof(a), "a");
    klee_make_symbolic(&b, sizeof(b), "b");
    klee_make_symbolic(&c, sizeof(c), "c");
    klee_make_symbolic(&d, sizeof(d), "d");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int result = 0;

    // Hot zone 1: many branches, same code
    if (noise[0] & 0x01) result++;
    if (noise[0] & 0x02) result++;
    if (noise[0] & 0x04) result++;
    if (noise[0] & 0x08) result++;
    if (noise[0] & 0x10) result++;
    if (noise[0] & 0x20) result++;

    // Cold spot 1: unique function behind gate
    if (a == 0x41) result = cold_fn_1(result);

    // Hot zone 2: same pattern, different byte
    if (noise[1] & 0x01) result++;
    if (noise[1] & 0x02) result++;
    if (noise[1] & 0x04) result++;
    if (noise[1] & 0x08) result++;
    if (noise[1] & 0x10) result++;
    if (noise[1] & 0x20) result++;

    // Cold spot 2
    if (b == 0x42) result = cold_fn_2(result);

    // Hot zone 3
    if (noise[2] & 0x01) result++;
    if (noise[2] & 0x02) result++;
    if (noise[2] & 0x04) result++;
    if (noise[2] & 0x08) result++;
    if (noise[2] & 0x10) result++;
    if (noise[2] & 0x20) result++;

    // Cold spot 3
    if (c == 0x43) result = cold_fn_3(result);

    // Hot zone 4
    if (noise[3] & 0x01) result++;
    if (noise[3] & 0x02) result++;
    if (noise[3] & 0x04) result++;
    if (noise[3] & 0x08) result++;
    if (noise[3] & 0x10) result++;
    if (noise[3] & 0x20) result++;

    // Cold spot 4
    if (d == 0x44) result = cold_fn_4(result);

    return result & 0xFF;
}
