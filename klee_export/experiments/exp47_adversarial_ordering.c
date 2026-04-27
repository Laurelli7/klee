// exp47: "Adversarial Ordering" — Tests whether DFS's deterministic
// left-first ordering can be catastrophically wrong.
//
// At each of 10 levels, the TRUE (left/first) branch leads to a
// massive state explosion (4 bitfield branches), while the FALSE
// branch (which DFS explores LAST) leads to the continuation.
// The final continuation has 5 unique functions.
//
// DFS: takes true at level 1 → explodes in 2^4 states, then true
//      at level 2 → 2^4 more, etc. Total: 2^40 states before reaching
//      ANY of the 5 prize functions.
// BFS: explores all levels simultaneously, reaches prizes faster.
// NURS: follows coverage signal toward prizes.
//
// Expected: All others >>> DFS
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int prize_0(int x) { return x + 100; }
__attribute__((noinline)) int prize_1(int x) { return x + 200; }
__attribute__((noinline)) int prize_2(int x) { return x + 300; }
__attribute__((noinline)) int prize_3(int x) { return x + 400; }
__attribute__((noinline)) int prize_4(int x) { return x + 500; }

int main() {
    uint8_t gate[10];
    uint8_t noise;
    klee_make_symbolic(gate, sizeof(gate), "gate");
    klee_make_symbolic(&noise, sizeof(noise), "noise");

    int r = 0;

    // Level 0
    if (gate[0] < 200) {  // TRUE side: DFS goes here first
        // Explosion: 4 bitfield branches
        if (noise & 0x01) r++;
        if (noise & 0x02) r++;
        if (noise & 0x04) r++;
        if (noise & 0x08) r++;
        return r;
    }
    // FALSE side: continuation (DFS reaches here last)

    // Level 1
    if (gate[1] < 200) {
        if (noise & 0x10) r++;
        if (noise & 0x20) r++;
        if (noise & 0x40) r++;
        if (noise & 0x80) r++;
        return r;
    }

    // Level 2
    if (gate[2] < 200) {
        if (gate[3] & 0x01) r++;
        if (gate[3] & 0x02) r++;
        if (gate[3] & 0x04) r++;
        if (gate[3] & 0x08) r++;
        return r;
    }

    // Level 3
    if (gate[4] < 200) {
        if (gate[5] & 0x01) r++;
        if (gate[5] & 0x02) r++;
        if (gate[5] & 0x04) r++;
        if (gate[5] & 0x08) r++;
        return r;
    }

    // Level 4
    if (gate[6] < 200) {
        if (gate[7] & 0x01) r++;
        if (gate[7] & 0x02) r++;
        if (gate[7] & 0x04) r++;
        if (gate[7] & 0x08) r++;
        return r;
    }

    // MADE IT THROUGH ALL 5 GATES → prizes
    if (gate[8] < 64)       return prize_0(r);
    else if (gate[8] < 128) return prize_1(r);
    else if (gate[8] < 192) return prize_2(r);
    else if (gate[9] < 128) return prize_3(r);
    else                     return prize_4(r);
}
