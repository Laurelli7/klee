// exp77: "Constraint Explosion" — Each branch ADDS a constraint on
// a SHARED variable, making later branches progressively harder.
// The first branch: "is x > 128?" — trivial.
// The fifth branch: "is x > 128 AND x < 200 AND x != 150 AND x%7==3?"
// — much harder.
//
// This creates a GRADIENT of solver difficulty along the path.
// States near the start have cheap queries; states near the end
// have expensive queries. NURS:qc should prefer early states.
//
// Combined with noise bytes to amplify the state count.
// The shared variable x accumulates constraints, while noise
// bytes create independent branching.
//
// Expected: qc prefers early states (cheap queries).
// covnew/md2u indifferent. DFS follows one path to the end.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int stage_0(int v) { return v + 10; }
__attribute__((noinline)) int stage_1(int v) { return v + 20; }
__attribute__((noinline)) int stage_2(int v) { return v + 30; }
__attribute__((noinline)) int stage_3(int v) { return v + 40; }
__attribute__((noinline)) int stage_4(int v) { return v + 50; }
__attribute__((noinline)) int stage_5(int v) { return v + 60; }
__attribute__((noinline)) int stage_6(int v) { return v + 70; }
__attribute__((noinline)) int stage_7(int v) { return v + 80; }
__attribute__((noinline)) int final_stage(int v) { return v + 99999; }

int main() {
    uint16_t x;  // shared variable accumulating constraints
    uint8_t noise[3];
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int result = 0;

    // Stage 0: simple constraint
    if (x > 32768) { result = stage_0(result); }
    else { result += 1; }

    // Noise between stages
    if (noise[0] & 0x01) result++;
    if (noise[0] & 0x02) result++;
    if (noise[0] & 0x04) result++;
    if (noise[0] & 0x08) result++;

    // Stage 1: tighter constraint
    if (x > 32768 && x < 49152) { result = stage_1(result); }
    else if (x > 49152) { result = stage_2(result); }

    if (noise[0] & 0x10) result++;
    if (noise[0] & 0x20) result++;
    if (noise[0] & 0x40) result++;
    if (noise[0] & 0x80) result++;

    // Stage 2: adds modular constraint
    if (x % 7 == 3) { result = stage_3(result); }
    if (x % 11 == 5) { result = stage_4(result); }

    if (noise[1] & 0x01) result++;
    if (noise[1] & 0x02) result++;
    if (noise[1] & 0x04) result++;
    if (noise[1] & 0x08) result++;

    // Stage 3: bit-level constraint
    if ((x & 0x00FF) == 0x42) { result = stage_5(result); }
    if ((x >> 8) == 0xAB) { result = stage_6(result); }

    if (noise[1] & 0x10) result++;
    if (noise[1] & 0x20) result++;
    if (noise[1] & 0x40) result++;
    if (noise[1] & 0x80) result++;

    // Stage 4: combined constraint — very expensive
    if (x > 32768 && x < 49152 && x % 7 == 3 && (x & 0xFF) == 0x42) {
        result = stage_7(result);
    }

    if (noise[2] & 0x01) result++;
    if (noise[2] & 0x02) result++;
    if (noise[2] & 0x04) result++;
    if (noise[2] & 0x08) result++;

    // Final: the ultimate constraint combination
    if (x == 0xAB42) {
        result = final_stage(result); // x = 0xAB42 satisfies ALL above
    }

    if (noise[2] & 0x10) result++;
    if (noise[2] & 0x20) result++;
    if (noise[2] & 0x40) result++;
    if (noise[2] & 0x80) result++;

    return result;
}
