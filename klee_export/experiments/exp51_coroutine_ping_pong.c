// exp51: "Coroutine Ping-Pong" — Two interleaved state machines where
// machine A's output feeds machine B's input and vice versa.
//
// Structure: Machine A processes odd-indexed inputs, Machine B processes
// even-indexed inputs. Each machine's result becomes a constraint on
// the next machine's branching.
//
// This creates INTERLEAVING: the state space is not a simple tree but
// a lattice where A's choices affect B and B's affect A.
//
// DFS: will commit to one interleaving, missing others
// BFS: explores all interleavings at each depth, state explosion
// random-path: randomly samples interleavings
// covnew: should find novel interleavings that reach new code
//
// Expected: random-path or covnew should do well; DFS commits too early;
// BFS explodes. The interleaving pattern is genuinely different from trees.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int machA_low(int x)  { return x + 0x10; }
__attribute__((noinline)) int machA_mid(int x)  { return x + 0x20; }
__attribute__((noinline)) int machA_high(int x) { return x + 0x30; }
__attribute__((noinline)) int machB_low(int x)  { return x * 2 + 1; }
__attribute__((noinline)) int machB_mid(int x)  { return x * 3 + 1; }
__attribute__((noinline)) int machB_high(int x) { return x * 5 + 1; }

__attribute__((noinline)) int finale_both_low(int x)  { return x + 1000; }
__attribute__((noinline)) int finale_both_high(int x) { return x + 2000; }
__attribute__((noinline)) int finale_mismatch(int x)  { return x + 3000; }

int main() {
    uint8_t input[8];
    klee_make_symbolic(input, sizeof(input), "input");

    int stateA = 0, stateB = 0;

    // Round 1: A processes input[0], result affects B's branching
    if (input[0] < 0x55)      stateA = machA_low(input[0]);
    else if (input[0] < 0xAA) stateA = machA_mid(input[0]);
    else                       stateA = machA_high(input[0]);

    // Round 1: B processes input[1], but threshold depends on A's output
    uint8_t threshB = (uint8_t)(stateA & 0x3F);  // B's threshold from A
    if (input[1] < threshB)      stateB = machB_low(input[1]);
    else if (input[1] < threshB + 0x40) stateB = machB_mid(input[1]);
    else                                 stateB = machB_high(input[1]);

    // Round 2: A processes input[2], threshold depends on B's output
    uint8_t threshA = (uint8_t)(stateB & 0x1F);
    if (input[2] < threshA)      stateA += machA_low(input[2]);
    else if (input[2] < threshA + 0x40) stateA += machA_mid(input[2]);
    else                                 stateA += machA_high(input[2]);

    // Round 2: B processes input[3], threshold depends on A's output
    threshB = (uint8_t)((stateA >> 2) & 0x3F);
    if (input[3] < threshB)      stateB += machB_low(input[3]);
    else if (input[3] < threshB + 0x40) stateB += machB_mid(input[3]);
    else                                 stateB += machB_high(input[3]);

    // Round 3: Same pattern with input[4] and input[5]
    threshA = (uint8_t)(stateB & 0x0F);
    if (input[4] < threshA)      stateA += machA_low(input[4]);
    else if (input[4] < threshA + 0x60) stateA += machA_mid(input[4]);
    else                                 stateA += machA_high(input[4]);

    threshB = (uint8_t)((stateA >> 3) & 0x1F);
    if (input[5] < threshB)      stateB += machB_low(input[5]);
    else if (input[5] < threshB + 0x50) stateB += machB_mid(input[5]);
    else                                 stateB += machB_high(input[5]);

    // Final: classify the combined result
    int total = stateA + stateB;
    if (stateA < 100 && stateB < 100) return finale_both_low(total);
    if (stateA > 500 && stateB > 500) return finale_both_high(total);
    return finale_mismatch(total);
}
