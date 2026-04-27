// exp72: "Distance vs Novelty Split" — Designed to split md2u from covnew.
//
// Structure:
//   After a shared preamble (noise), the program forks:
//   Path NOVEL: 5 unique breadcrumb functions at INCREASING distance
//     from uncovered code. Each breadcrumb is 1 step, but the REAL target
//     (deep_prize) is 20 steps away after the breadcrumbs.
//   Path CLOSE: 5 steps of ALREADY-COVERED code (shared_step), but the
//     target (close_prize) is only 2 steps away.
//
// covnew: should chase Path NOVEL (5 unique breadcrumbs = coverage novelty)
// md2u: should chase Path CLOSE (target is 2 steps away vs 20)
// qc: no preference (equal solver cost)
//
// This should produce: md2u picks close_prize, covnew picks breadcrumbs.
#include "klee/klee.h"
#include <stdint.h>

// Breadcrumbs: novel but far from prize
__attribute__((noinline)) int bc_0(int x) { return x | 0x01; }
__attribute__((noinline)) int bc_1(int x) { return x | 0x02; }
__attribute__((noinline)) int bc_2(int x) { return x | 0x04; }
__attribute__((noinline)) int bc_3(int x) { return x | 0x08; }
__attribute__((noinline)) int bc_4(int x) { return x | 0x10; }

// Filler: already-covered code (no novelty)
__attribute__((noinline)) int shared_step(int x) { return x + 1; }

// Prizes
__attribute__((noinline)) int deep_prize(int x) { return x + 77777; }
__attribute__((noinline)) int close_prize(int x) { return x + 88888; }

int main() {
    uint8_t choice;
    uint8_t gate;
    uint8_t noise[3];
    klee_make_symbolic(&choice, sizeof(choice), "choice");
    klee_make_symbolic(&gate, sizeof(gate), "gate");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int result = 0;

    // Noise preamble: amplify states
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

    // Force shared_step to be "already covered" by calling it before the fork
    result = shared_step(result);
    result = shared_step(result);

    if (choice & 0x01) {
        // Path NOVEL: breadcrumbs then a long chain to deep_prize
        result = bc_0(result);
        result = bc_1(result);
        result = bc_2(result);
        result = bc_3(result);
        result = bc_4(result);

        // Long chain of shared steps before prize (20 steps)
        for (int i = 0; i < 20; i++) {
            result = shared_step(result);
        }

        // Deep prize: behind a gate
        if (gate == 0xAA) {
            result = deep_prize(result);
        }
    } else {
        // Path CLOSE: boring shared steps, but close_prize is near
        result = shared_step(result);
        result = shared_step(result);

        // Close prize: right here, behind a gate
        if (gate == 0xBB) {
            result = close_prize(result);
        }
    }

    return result;
}
