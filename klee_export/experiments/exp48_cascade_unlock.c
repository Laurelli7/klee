// exp48: "Cascade Unlock" — A lock-and-key puzzle where each key
// unlocks the next region. Tests which searcher finds the single
// correct path through a sequence of equality checks fastest.
//
// Structure: key[0] must equal 0xAA to unlock region 1 (unique fn).
// key[1] must equal 0xBB to unlock region 2 (unique fn), etc.
// Each region also has 4 branches of noise after the unlock.
//
// DFS: if key[0]'s first fork happens to try 0xAA, it goes deep.
//      Otherwise, it explores all 255 wrong values first. ~50% chance of
//      being great or terrible.
// NURS:covnew: Should chase the coverage from each unlock.
// BFS: tries all 256 values of key[0] at depth 1, finds 0xAA, then
//      tries all 256 values of key[1] at depth 2, etc. Methodical.
//
// Expected: NURS:covnew should excel (each unlock provides strong
// coverage signal). BFS should be decent. DFS unpredictable.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int region_0(int x) { return x + 111; }
__attribute__((noinline)) int region_1(int x) { return x + 222; }
__attribute__((noinline)) int region_2(int x) { return x + 333; }
__attribute__((noinline)) int region_3(int x) { return x + 444; }
__attribute__((noinline)) int final_boss(int x) { return x + 999; }

int main() {
    uint8_t key[4];
    uint8_t noise;
    klee_make_symbolic(key, sizeof(key), "key");
    klee_make_symbolic(&noise, sizeof(noise), "noise");

    int r = 0;

    // Lock 0: key must be 0xAA
    if (key[0] != 0xAA) return 0;
    r = region_0(r);
    // Noise after unlock
    if (noise & 0x01) r++;
    if (noise & 0x02) r++;

    // Lock 1: key must be 0xBB
    if (key[1] != 0xBB) return r;
    r = region_1(r);
    if (noise & 0x04) r++;
    if (noise & 0x08) r++;

    // Lock 2: key must be 0xCC
    if (key[2] != 0xCC) return r;
    r = region_2(r);
    if (noise & 0x10) r++;
    if (noise & 0x20) r++;

    // Lock 3: key must be 0xDD
    if (key[3] != 0xDD) return r;
    r = region_3(r);
    if (noise & 0x40) r++;
    if (noise & 0x80) r++;

    // Ultimate: all 4 keys correct
    return final_boss(r);
}
