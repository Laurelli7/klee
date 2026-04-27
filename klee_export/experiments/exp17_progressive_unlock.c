// exp17: "Progressive unlock" — Code that reveals itself layer by layer.
// Each "layer" has unique functions. NURS:covnew should greedily
// chase each new layer. random-path should stumble into them.
// BFS should find them at each depth level.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int layer1_fn(int x) { return x + 10; }
__attribute__((noinline)) int layer2_fn(int x) { return x + 20; }
__attribute__((noinline)) int layer3_fn(int x) { return x + 30; }
__attribute__((noinline)) int layer4_fn(int x) { return x + 40; }
__attribute__((noinline)) int layer5_fn(int x) { return x + 50; }

int main() {
    uint8_t keys[5];
    uint8_t filler[3]; // creates state explosion between layers
    klee_make_symbolic(keys, sizeof(keys), "keys");
    klee_make_symbolic(filler, sizeof(filler), "filler");

    int result = 0;

    // Filler 1: 3 bits of branching
    if (filler[0] & 0x01) result++;
    if (filler[0] & 0x02) result++;
    if (filler[0] & 0x04) result++;

    // Layer 1
    if (keys[0] == 0x11) result = layer1_fn(result);

    // Filler 2: 3 more bits
    if (filler[1] & 0x01) result++;
    if (filler[1] & 0x02) result++;
    if (filler[1] & 0x04) result++;

    // Layer 2
    if (keys[1] == 0x22) result = layer2_fn(result);

    // Filler 3: 3 more bits
    if (filler[2] & 0x01) result++;
    if (filler[2] & 0x02) result++;
    if (filler[2] & 0x04) result++;

    // Layer 3
    if (keys[2] == 0x33) result = layer3_fn(result);

    // Layer 4 (no filler — tests immediate sequential unlock)
    if (keys[3] == 0x44) result = layer4_fn(result);

    // Layer 5
    if (keys[4] == 0x55) result = layer5_fn(result);

    return result;
}
