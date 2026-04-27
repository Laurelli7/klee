// exp56: "Error Cascade" — Multiple assertion failures at different
// depths. Tests which searcher finds the most DISTINCT errors.
//
// Structure: 5-stage pipeline. Each stage can fail (assert) based
// on different symbolic conditions. Stage 1 fails easily (wide condition),
// Stage 5 fails rarely (narrow condition). Between stages, noise branches
// create state bloat that obscures the error paths.
//
// The interesting metric is not just coverage but ERROR COUNT.
// DFS: finds stage 5 first (deep), then backtracks
// BFS: finds stage 1 first (shallow), but may never reach stage 5
// covnew: each assertion handler is "new" coverage 
// md2u: should track distance to each assert independently
//
// Expected: DFS finds deeper errors, BFS finds shallower ones faster.
// The TIME to find all 5 distinct errors should differ dramatically.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) void stage1_fail(void) { klee_assert(0); }
__attribute__((noinline)) void stage2_fail(void) { klee_assert(0); }
__attribute__((noinline)) void stage3_fail(void) { klee_assert(0); }
__attribute__((noinline)) void stage4_fail(void) { klee_assert(0); }
__attribute__((noinline)) void stage5_fail(void) { klee_assert(0); }

__attribute__((noinline)) int stage_pass(int v) { return v + 1; }

int main() {
    uint8_t input[5];
    uint8_t noise[2];
    klee_make_symbolic(input, sizeof(input), "input");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int pipeline = 0;

    // Stage 1: fails if input[0] is exactly 0x42
    if (input[0] == 0x42) stage1_fail();
    pipeline = stage_pass(pipeline);

    // Noise between stages
    if (noise[0] & 0x01) pipeline++;
    if (noise[0] & 0x02) pipeline++;
    if (noise[0] & 0x04) pipeline++;
    if (noise[0] & 0x08) pipeline++;

    // Stage 2: fails if input[1] is in range [0x30, 0x3F]
    if (input[1] >= 0x30 && input[1] <= 0x3F) stage2_fail();
    pipeline = stage_pass(pipeline);

    if (noise[0] & 0x10) pipeline++;
    if (noise[0] & 0x20) pipeline++;

    // Stage 3: fails if input[2]'s top 3 bits are 101
    if ((input[2] >> 5) == 0x05) stage3_fail();
    pipeline = stage_pass(pipeline);

    if (noise[0] & 0x40) pipeline++;
    if (noise[0] & 0x80) pipeline++;

    // Stage 4: fails if input[3] AND input[0] have same parity
    if ((input[3] & 1) == (input[0] & 1)) stage4_fail();
    pipeline = stage_pass(pipeline);

    if (noise[1] & 0x01) pipeline++;
    if (noise[1] & 0x02) pipeline++;
    if (noise[1] & 0x04) pipeline++;
    if (noise[1] & 0x08) pipeline++;

    // Stage 5: fails ONLY if all inputs satisfy a joint condition
    if ((input[0] ^ input[1] ^ input[2] ^ input[3] ^ input[4]) == 0xAA)
        stage5_fail();

    return pipeline;
}
