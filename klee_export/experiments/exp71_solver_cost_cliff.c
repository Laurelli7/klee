// exp71: "Solver Cost Cliff" — Designed to split NURS:qc from covnew/md2u.
//
// The program has TWO paths after an initial fork:
//   Path CHEAP: 10 simple equality checks, each behind a unique function.
//     Solver: O(1) per query (just equality).
//   Path EXPENSIVE: 10 multiplication/division chains, each behind a unique
//     function. Solver: O(expensive) per query (nonlinear arithmetic).
//
// Both paths have EQUAL coverage novelty (10 unique functions each).
// Both paths have EQUAL distance-to-uncovered.
// But CHEAP costs 100× less solver time per query.
//
// NURS:qc should STRONGLY prefer Path CHEAP (lower query cost).
// NURS:covnew should be INDIFFERENT (same coverage on both paths).
// NURS:md2u should be INDIFFERENT (same distance on both paths).
//
// The noise bytes AFTER the choice amplify the state count to fill
// the time budget (2^20 = 1M states from 2.5 bytes × 8 bits).
#include "klee/klee.h"
#include <stdint.h>

// Cheap-path handlers
__attribute__((noinline)) int cheap_0(int x) { return x + 10; }
__attribute__((noinline)) int cheap_1(int x) { return x + 20; }
__attribute__((noinline)) int cheap_2(int x) { return x + 30; }
__attribute__((noinline)) int cheap_3(int x) { return x + 40; }
__attribute__((noinline)) int cheap_4(int x) { return x + 50; }

// Expensive-path handlers
__attribute__((noinline)) int expen_0(int x) { return x + 100; }
__attribute__((noinline)) int expen_1(int x) { return x + 200; }
__attribute__((noinline)) int expen_2(int x) { return x + 300; }
__attribute__((noinline)) int expen_3(int x) { return x + 400; }
__attribute__((noinline)) int expen_4(int x) { return x + 500; }

int main() {
    uint8_t choice;
    uint8_t cheap_keys[5];
    uint8_t expen_keys[5];
    uint8_t noise[3]; // post-choice noise
    klee_make_symbolic(&choice, sizeof(choice), "choice");
    klee_make_symbolic(cheap_keys, sizeof(cheap_keys), "ckeys");
    klee_make_symbolic(expen_keys, sizeof(expen_keys), "ekeys");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int result = 0;

    if (choice & 0x01) {
        // CHEAP PATH: simple equality checks
        if (cheap_keys[0] == 0x11) result = cheap_0(result);
        if (cheap_keys[1] == 0x22) result = cheap_1(result);
        if (cheap_keys[2] == 0x33) result = cheap_2(result);
        if (cheap_keys[3] == 0x44) result = cheap_3(result);
        if (cheap_keys[4] == 0x55) result = cheap_4(result);
    } else {
        // EXPENSIVE PATH: nonlinear arithmetic constraints
        // Each condition involves multiplication, creating expensive SMT queries
        uint16_t k0 = (uint16_t)expen_keys[0] * (uint16_t)expen_keys[1];
        uint16_t k1 = (uint16_t)expen_keys[1] * (uint16_t)expen_keys[2];
        uint16_t k2 = (uint16_t)expen_keys[2] * (uint16_t)expen_keys[3];
        uint16_t k3 = (uint16_t)expen_keys[3] * (uint16_t)expen_keys[4];
        uint16_t k4 = (uint16_t)expen_keys[4] * (uint16_t)expen_keys[0];

        if (k0 == 0x1234) result = expen_0(result);
        if (k1 == 0x5678) result = expen_1(result);
        if (k2 == 0x9ABC) result = expen_2(result);
        if (k3 == 0xDEF0) result = expen_3(result);
        if (k4 == 0x1111) result = expen_4(result);
    }

    // Noise amplifier: creates states to fill the time budget
    for (int i = 0; i < 3; i++) {
        if (noise[i] & 0x01) result++;
        if (noise[i] & 0x02) result++;
        if (noise[i] & 0x04) result++;
        if (noise[i] & 0x08) result++;
        if (noise[i] & 0x10) result++;
        if (noise[i] & 0x20) result++;
        if (noise[i] & 0x40) result++;
        if (noise[i] & 0x80) result++;
    }

    return result;
}
