// exp94: "State Flood Trap" — An early branch creates a massive state
// explosion (fork bomb) while a SECOND independent region has unique
// shallow coverage. The fork bomb region uses the SAME function body
// (no new coverage to chase). DFS enters the fork bomb first and never
// escapes. NURS variants may also get overwhelmed by the sheer number
// of fork-bomb states diluting any heuristic signal from the shallow region.
//
// BFS explores both the fork bomb AND shallow region at depth 1,
// discovering the shallow unique code before the fork bomb explodes.
//
// Structure:
// - Two symbolic bytes: A (fork bomb driver), B (coverage selector)
// - Step 1: check bit 0 of A → goes to fork region or coverage region
// - Fork region: cascade of independent bit checks on A (no new coverage)
// - Coverage region: 8-way switch on B → 8 unique functions
//
// Expected: BFS covers all unique functions; DFS gets trapped in fork region.
#include "klee/klee.h"
#include <stdint.h>

// Unique coverage functions
__attribute__((noinline)) int cover_1(int x) { return x * 2 + 11; }
__attribute__((noinline)) int cover_2(int x) { return x * 3 + 22; }
__attribute__((noinline)) int cover_3(int x) { return x * 5 + 33; }
__attribute__((noinline)) int cover_4(int x) { return x * 7 + 44; }
__attribute__((noinline)) int cover_5(int x) { return x * 11 + 55; }
__attribute__((noinline)) int cover_6(int x) { return x * 13 + 66; }
__attribute__((noinline)) int cover_7(int x) { return x * 17 + 77; }
__attribute__((noinline)) int cover_8(int x) { return x * 19 + 88; }

// Fork bomb function — called repeatedly, no new coverage after first call
__attribute__((noinline)) int bomb_step(int acc, int bit) {
    return acc + bit;
}

int main() {
    uint8_t A, B;
    uint8_t extra[3]; // extra symbolic bytes for deeper forking
    klee_make_symbolic(&A, sizeof(A), "A");
    klee_make_symbolic(&B, sizeof(B), "B");
    klee_make_symbolic(extra, sizeof(extra), "extra");

    int r = 0;

    if (A & 0x01) {
        // FORK BOMB REGION: 24 independent bit branches
        // All use bomb_step → no new CoveredInstr after first usage
        r = bomb_step(r, (A >> 1) & 1);
        r = bomb_step(r, (A >> 2) & 1);
        r = bomb_step(r, (A >> 3) & 1);
        r = bomb_step(r, (A >> 4) & 1);
        r = bomb_step(r, (A >> 5) & 1);
        r = bomb_step(r, (A >> 6) & 1);
        r = bomb_step(r, (A >> 7) & 1);
        for (int i = 0; i < 3; i++) {
            r = bomb_step(r, (extra[i] >> 0) & 1);
            r = bomb_step(r, (extra[i] >> 1) & 1);
            r = bomb_step(r, (extra[i] >> 2) & 1);
            r = bomb_step(r, (extra[i] >> 3) & 1);
            r = bomb_step(r, (extra[i] >> 4) & 1);
            r = bomb_step(r, (extra[i] >> 5) & 1);
            r = bomb_step(r, (extra[i] >> 6) & 1);
            r = bomb_step(r, (extra[i] >> 7) & 1);
        }
    } else {
        // COVERAGE REGION: 8 unique functions
        switch (B >> 5) {
            case 0: r = cover_1(B); break;
            case 1: r = cover_2(B); break;
            case 2: r = cover_3(B); break;
            case 3: r = cover_4(B); break;
            case 4: r = cover_5(B); break;
            case 5: r = cover_6(B); break;
            case 6: r = cover_7(B); break;
            case 7: r = cover_8(B); break;
        }
        // Coverage region also has a small tail to be realistic
        if (extra[0] & 0x01) r++;
        if (extra[0] & 0x02) r++;
        if (extra[0] & 0x04) r++;
        if (extra[0] & 0x08) r++;
    }

    return r;
}
