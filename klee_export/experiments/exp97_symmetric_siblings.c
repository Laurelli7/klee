// exp97: "Symmetric Siblings" — All branches have IDENTICAL structure
// (same depth, same complexity) but different unique code at each leaf.
// No branch is "closer to uncovered" than any other, so covnew/md2u
// can't gain an advantage. BFS systematically visits all siblings
// at each depth, while DFS goes deep in one branch.
//
// Structure: Binary tree of depth 4 on 4 symbolic bits.
// Each of the 16 leaves calls a unique function.
// After the leaf function, a shared deep tail (20 branches) traps DFS.
//
// Key: Perfect symmetry means covnew/md2u degenerate to random selection.
// BFS is the only strategy that GUARANTEES covering all 16 leaves quickly.
//
// Expected: BFS covers all 16 unique functions before timeout;
// DFS covers 1; random searchers cover ~8.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int leaf_00(int x) { return x * 2 + 0x100; }
__attribute__((noinline)) int leaf_01(int x) { return x * 3 + 0x101; }
__attribute__((noinline)) int leaf_02(int x) { return x * 5 + 0x102; }
__attribute__((noinline)) int leaf_03(int x) { return x * 7 + 0x103; }
__attribute__((noinline)) int leaf_04(int x) { return x * 11 + 0x104; }
__attribute__((noinline)) int leaf_05(int x) { return x * 13 + 0x105; }
__attribute__((noinline)) int leaf_06(int x) { return x * 17 + 0x106; }
__attribute__((noinline)) int leaf_07(int x) { return x * 19 + 0x107; }
__attribute__((noinline)) int leaf_08(int x) { return x * 23 + 0x108; }
__attribute__((noinline)) int leaf_09(int x) { return x * 29 + 0x109; }
__attribute__((noinline)) int leaf_10(int x) { return x * 31 + 0x10A; }
__attribute__((noinline)) int leaf_11(int x) { return x * 37 + 0x10B; }
__attribute__((noinline)) int leaf_12(int x) { return x * 41 + 0x10C; }
__attribute__((noinline)) int leaf_13(int x) { return x * 43 + 0x10D; }
__attribute__((noinline)) int leaf_14(int x) { return x * 47 + 0x10E; }
__attribute__((noinline)) int leaf_15(int x) { return x * 53 + 0x10F; }

int main() {
    uint8_t sel;
    uint8_t tail[5]; // 5 bytes = 40 binary branches in tail
    klee_make_symbolic(&sel, sizeof(sel), "sel");
    klee_make_symbolic(tail, sizeof(tail), "tail");

    int r = 0;

    // Binary tree on bits 7..4 of sel (depth 4, 16 leaves)
    // Using nested ifs to create explicit tree structure
    if (sel & 0x80) {
        if (sel & 0x40) {
            if (sel & 0x20) {
                if (sel & 0x10) {
                    r = leaf_15(sel);
                } else {
                    r = leaf_14(sel);
                }
            } else {
                if (sel & 0x10) {
                    r = leaf_13(sel);
                } else {
                    r = leaf_12(sel);
                }
            }
        } else {
            if (sel & 0x20) {
                if (sel & 0x10) {
                    r = leaf_11(sel);
                } else {
                    r = leaf_10(sel);
                }
            } else {
                if (sel & 0x10) {
                    r = leaf_09(sel);
                } else {
                    r = leaf_08(sel);
                }
            }
        }
    } else {
        if (sel & 0x40) {
            if (sel & 0x20) {
                if (sel & 0x10) {
                    r = leaf_07(sel);
                } else {
                    r = leaf_06(sel);
                }
            } else {
                if (sel & 0x10) {
                    r = leaf_05(sel);
                } else {
                    r = leaf_04(sel);
                }
            }
        } else {
            if (sel & 0x20) {
                if (sel & 0x10) {
                    r = leaf_03(sel);
                } else {
                    r = leaf_02(sel);
                }
            } else {
                if (sel & 0x10) {
                    r = leaf_01(sel);
                } else {
                    r = leaf_00(sel);
                }
            }
        }
    }

    // Deep tail — shared by ALL leaves, no new coverage
    for (int i = 0; i < 5; i++) {
        if (tail[i] & 0x01) r ^= 1;
        if (tail[i] & 0x02) r ^= 2;
        if (tail[i] & 0x04) r ^= 4;
        if (tail[i] & 0x08) r ^= 8;
        if (tail[i] & 0x10) r ^= 16;
        if (tail[i] & 0x20) r ^= 32;
        if (tail[i] & 0x40) r ^= 64;
        if (tail[i] & 0x80) r ^= 128;
    }

    return r;
}
