// exp110: "Collision Branching" — Multiple symbolic writes to a small
// table, then EXPLICIT branches checking which slots collided.
// Each collision combination calls a unique handler.
//
// With 4 writes to a 4-slot table, there are many possible collision
// patterns (no collisions, 2 collide, 3 collide, all collide, etc).
// BFS explores all first-write targets before second writes → diverse
// collision states. DFS follows one collision sequence. covnew/md2u
// can't distinguish the coverage value of different collision patterns.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int no_collision(int x) { return x + 100; }
__attribute__((noinline)) int coll_01(int x) { return x + 201; }
__attribute__((noinline)) int coll_02(int x) { return x + 202; }
__attribute__((noinline)) int coll_03(int x) { return x + 203; }
__attribute__((noinline)) int coll_12(int x) { return x + 212; }
__attribute__((noinline)) int coll_13(int x) { return x + 213; }
__attribute__((noinline)) int coll_23(int x) { return x + 223; }
__attribute__((noinline)) int triple_coll(int x) { return x + 300; }
__attribute__((noinline)) int quad_coll(int x) { return x + 400; }
__attribute__((noinline)) int pair_pair(int x) { return x + 500; }

int main() {
    uint8_t w[4]; // 4 symbolic write indices
    klee_make_symbolic(w, sizeof(w), "w");

    // Each write targets a slot in a 4-element table
    int slot[4] = {0, 0, 0, 0};
    int s0 = w[0] & 0x03;
    int s1 = w[1] & 0x03;
    int s2 = w[2] & 0x03;
    int s3 = w[3] & 0x03;

    // Perform writes (mark slots)
    slot[s0] = 1;
    slot[s1] += 2;
    slot[s2] += 4;
    slot[s3] += 8;

    int r = 0;

    // Check pairwise collisions
    int c01 = (s0 == s1);
    int c02 = (s0 == s2);
    int c03 = (s0 == s3);
    int c12 = (s1 == s2);
    int c13 = (s1 == s3);
    int c23 = (s2 == s3);

    int num_coll = c01 + c02 + c03 + c12 + c13 + c23;

    if (num_coll == 0) {
        // No collisions at all (impossible with 4 writes to 4 slots
        // when all different — this IS possible: s0≠s1≠s2≠s3)
        r = no_collision(r);
    } else if (num_coll == 1) {
        // Exactly one pair collides
        if (c01) r = coll_01(r);
        else if (c02) r = coll_02(r);
        else if (c03) r = coll_03(r);
        else if (c12) r = coll_12(r);
        else if (c13) r = coll_13(r);
        else r = coll_23(r);
    } else if (num_coll == 3) {
        // Triple collision (3 writes same slot)
        r = triple_coll(r);
    } else if (num_coll == 6) {
        // All 4 writes same slot
        r = quad_coll(r);
    } else {
        // Two separate pairs collide
        r = pair_pair(r);
    }

    // Additional coverage: branch on which slots are occupied
    int occupied = 0;
    for (int i = 0; i < 4; i++) {
        if (slot[i] > 0) occupied++;
    }

    // More unique handlers based on occupation count
    switch (occupied) {
        case 1: r += 10; break;
        case 2: r += 20; break;
        case 3: r += 30; break;
        case 4: r += 40; break;
    }

    return r;
}
