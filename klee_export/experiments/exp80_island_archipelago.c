// exp80: "Coverage Island Archipelago" — 16 isolated "islands" of unique
// code, each behind a DIFFERENT type of gate with DIFFERENT cost:
//
//   Islands 0-3: behind simple equality (cheap solver)
//   Islands 4-7: behind range checks (medium solver)
//   Islands 8-11: behind modular arithmetic (expensive solver)
//   Islands 12-15: behind multi-variable constraints (very expensive solver)
//
// Each island has the SAME amount of unique code (1 function).
// But the COST to reach each island differs dramatically.
//
// NURS:qc should prefer islands 0-3 (cheapest to reach).
// NURS:covnew should be indifferent (all islands = same coverage novelty).
// NURS:md2u should be indifferent (all islands = same distance).
//
// Between islands, noise amplifies state count.
// With 2 bytes noise (16 branches), we get 2^16 = 65536 base states.
// Each island's gate creates an additional fork.
//
// Expected: qc reaches cheap islands 0-3 fastest.
// covnew/md2u reach all islands with equal probability.
// If qc reaches MORE cheap islands in the budget, we see the split.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int island_0(int x)  { return x + 1000; }
__attribute__((noinline)) int island_1(int x)  { return x + 1001; }
__attribute__((noinline)) int island_2(int x)  { return x + 1002; }
__attribute__((noinline)) int island_3(int x)  { return x + 1003; }
__attribute__((noinline)) int island_4(int x)  { return x + 1004; }
__attribute__((noinline)) int island_5(int x)  { return x + 1005; }
__attribute__((noinline)) int island_6(int x)  { return x + 1006; }
__attribute__((noinline)) int island_7(int x)  { return x + 1007; }
__attribute__((noinline)) int island_8(int x)  { return x + 1008; }
__attribute__((noinline)) int island_9(int x)  { return x + 1009; }
__attribute__((noinline)) int island_10(int x) { return x + 1010; }
__attribute__((noinline)) int island_11(int x) { return x + 1011; }
__attribute__((noinline)) int island_12(int x) { return x + 1012; }
__attribute__((noinline)) int island_13(int x) { return x + 1013; }
__attribute__((noinline)) int island_14(int x) { return x + 1014; }
__attribute__((noinline)) int island_15(int x) { return x + 1015; }

int main() {
    uint8_t keys[4];     // 4 keys for different gate types
    uint8_t noise[2];    // noise amplifier
    klee_make_symbolic(keys, sizeof(keys), "keys");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int result = 0;

    // Noise: 2^16 base states
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

    // CHEAP ISLANDS (simple equality)
    if (keys[0] == 0x10) result = island_0(result);
    if (keys[0] == 0x20) result = island_1(result);
    if (keys[0] == 0x30) result = island_2(result);
    if (keys[0] == 0x40) result = island_3(result);

    // MEDIUM ISLANDS (range checks)
    if (keys[1] > 0x10 && keys[1] < 0x20) result = island_4(result);
    if (keys[1] > 0x40 && keys[1] < 0x50) result = island_5(result);
    if (keys[1] > 0x80 && keys[1] < 0x90) result = island_6(result);
    if (keys[1] > 0xC0 && keys[1] < 0xD0) result = island_7(result);

    // EXPENSIVE ISLANDS (modular arithmetic)
    if (keys[2] % 17 == 3)  result = island_8(result);
    if (keys[2] % 19 == 7)  result = island_9(result);
    if (keys[2] % 23 == 11) result = island_10(result);
    if (keys[2] % 29 == 13) result = island_11(result);

    // VERY EXPENSIVE ISLANDS (multi-variable constraints)
    uint16_t combo = (uint16_t)keys[2] * (uint16_t)keys[3];
    if (combo == 0x1234) result = island_12(result);
    if (combo == 0x5678) result = island_13(result);
    if (combo == 0x9ABC) result = island_14(result);
    if (combo == 0xDEF0) result = island_15(result);

    return result;
}
