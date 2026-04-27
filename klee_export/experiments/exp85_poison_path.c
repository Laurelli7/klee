// exp85: "Poison Path" — Adversarial structure where the
// high-coverage path is deliberately made EXPENSIVE, and the
// low-coverage path is deliberately made CHEAP but valuable.
//
// Structure:
//   Path A: 8 novel functions but each guarded by a modular
//           arithmetic constraint that's O(2^n) for the solver.
//   Path B: 2 novel functions behind trivial equality checks.
//           BUT path B leads to an error assertion (the "target").
//
// covnew will chase Path A's 8 novel functions, spending all its
// solver budget on expensive constraints and never reaching the error.
// qc will see Path A's queries are expensive and prefer Path B.
// md2u will see Path B reaches uncovered error-handling code.
//
// This is adversarial to covnew specifically.
#include "klee/klee.h"
#include <stdint.h>

// Poison functions - novel but expensive to reach
__attribute__((noinline)) int poison_0(int x) { return x + 0xDE00; }
__attribute__((noinline)) int poison_1(int x) { return x + 0xDE01; }
__attribute__((noinline)) int poison_2(int x) { return x + 0xDE02; }
__attribute__((noinline)) int poison_3(int x) { return x + 0xDE03; }
__attribute__((noinline)) int poison_4(int x) { return x + 0xDE04; }
__attribute__((noinline)) int poison_5(int x) { return x + 0xDE05; }
__attribute__((noinline)) int poison_6(int x) { return x + 0xDE06; }
__attribute__((noinline)) int poison_7(int x) { return x + 0xDE07; }

// Target functions - cheap and contain the bug
__attribute__((noinline)) int target_entry(int x) { return x + 0xBEEF; }
__attribute__((noinline)) int target_bug(int x) { return x + 0xDEAD; }

int main() {
    uint16_t key;        // main dispatch
    uint8_t noise[2];    // state amplifier
    uint8_t trigger;     // target trigger
    klee_make_symbolic(&key, sizeof(key), "key");
    klee_make_symbolic(noise, sizeof(noise), "noise");
    klee_make_symbolic(&trigger, sizeof(trigger), "trigger");

    int result = 0;

    // Noise preamble
    for (int i = 0; i < 2; i++) {
        if (noise[i] & 0x01) result++;
        if (noise[i] & 0x02) result++;
        if (noise[i] & 0x04) result++;
        if (noise[i] & 0x08) result++;
        if (noise[i] & 0x10) result++;
        if (noise[i] & 0x20) result++;
        if (noise[i] & 0x40) result++;
        if (noise[i] & 0x80) result++;
    }

    // PATH A: 8 poison islands (expensive solver, lots of coverage)
    if (key % 97 == 13) result = poison_0(result);
    if (key % 89 == 17) result = poison_1(result);
    if (key % 83 == 23) result = poison_2(result);
    if (key % 79 == 29) result = poison_3(result);
    if ((key * key) % 101 == 42) result = poison_4(result);
    if ((key * key) % 103 == 37) result = poison_5(result);
    if ((key ^ 0x5A5A) % 107 == 51) result = poison_6(result);
    if ((key ^ 0xA5A5) % 109 == 53) result = poison_7(result);

    // PATH B: cheap target path (equality check = trivial for solver)
    if (trigger == 0x42) {
        result = target_entry(result);
        // The bug is deeper — requires both trigger AND a key range
        if (key > 100 && key < 200) {
            result = target_bug(result);
            klee_assert(result != 0xDEAD);  // The actual target
        }
    }

    return result;
}
