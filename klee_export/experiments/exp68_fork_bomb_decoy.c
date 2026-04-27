// exp68: "Fork Bomb Decoy" — Tests state management under extreme
// forking pressure from one path, while a parallel path has a bug.
//
// Structure: Two paths from initial fork.
//   Path A: "Fork bomb" — 4 symbolic bytes with bit-testing creates
//           2^32 theoretical states (in practice, limited by timeout).
//           No interesting coverage.
//   Path B: "Bug path" — A short chain of 3 equality checks leading
//           to klee_assert(0). Total states: ~768 (3 × 256).
//
// The question: which searcher reaches the bug on Path B fastest
// while Path A is flooding the state pool?
//
// DFS: if it takes Path A first, drowns. If Path B first, finds bug instantly.
// BFS: creates both Path A and Path B at depth 1, then explores both.
//      Path A's breadth dominates, starving Path B.
// covnew: Path B has unique code (assertion handler), should prioritize it.
// random-path: 50% chance of going to B.
// qc: Path B queries are cheap (equalities), Path A queries are cheap too.
//
// Expected: covnew should win (unique code = assertion).
// DFS is 50/50. BFS should be slow (Path A starves Path B).
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int bomb_handler(int x) { return x + 1; }
__attribute__((noinline)) int bug_stage1(int x) { return x + 100; }
__attribute__((noinline)) int bug_stage2(int x) { return x + 200; }
__attribute__((noinline)) int bug_found(int x) { return x + 9999; }

int main() {
    uint8_t fork_choice;
    klee_make_symbolic(&fork_choice, sizeof(fork_choice), "fork");

    if (fork_choice & 0x01) {
        // Path A: Fork bomb
        uint8_t bomb[4];
        klee_make_symbolic(bomb, sizeof(bomb), "bomb");

        int result = 0;
        for (int i = 0; i < 4; i++) {
            if (bomb[i] & 0x01) result = bomb_handler(result);
            if (bomb[i] & 0x02) result = bomb_handler(result);
            if (bomb[i] & 0x04) result = bomb_handler(result);
            if (bomb[i] & 0x08) result = bomb_handler(result);
            if (bomb[i] & 0x10) result = bomb_handler(result);
            if (bomb[i] & 0x20) result = bomb_handler(result);
            if (bomb[i] & 0x40) result = bomb_handler(result);
            if (bomb[i] & 0x80) result = bomb_handler(result);
        }
        return result;
    } else {
        // Path B: Bug path (short, with unique stages)
        uint8_t key[3];
        klee_make_symbolic(key, sizeof(key), "key");

        int r = 0;
        if (key[0] == 0x42) {
            r = bug_stage1(r);
            if (key[1] == 0x43) {
                r = bug_stage2(r);
                if (key[2] == 0x44) {
                    r = bug_found(r);
                    klee_assert(0);  // THE BUG
                }
            }
        }
        return r;
    }
}
