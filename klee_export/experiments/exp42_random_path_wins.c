// exp42: "Random-path wins" — Designed to favor random-path over DFS/BFS.
// Structure: A deep binary tree where ONLY the leaves matter (each leaf
// has a unique function call). The tree is perfectly balanced so no
// heuristic signal differentiates branches. Random-path's tree-based
// uniform random selection should find leaves fastest because it gives
// equal probability to each subtree regardless of how many states
// are pending in each subtree.
//
// DFS: goes all the way left first, slow to reach right leaves
// BFS: processes all nodes at each depth, slow to reach leaves at all
// random-path: jumps across the tree, reaches diverse leaves fast
// NURS: no coverage signal until leaves, effectively random
//
// Tree: 4 levels deep → 16 leaves, each calling a unique function.
// After the tree, a bitfield tail to waste time for non-leaf-reaching searchers.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int leaf_00(int x) { return x + 1000; }
__attribute__((noinline)) int leaf_01(int x) { return x + 1001; }
__attribute__((noinline)) int leaf_02(int x) { return x + 1002; }
__attribute__((noinline)) int leaf_03(int x) { return x + 1003; }
__attribute__((noinline)) int leaf_04(int x) { return x + 1004; }
__attribute__((noinline)) int leaf_05(int x) { return x + 1005; }
__attribute__((noinline)) int leaf_06(int x) { return x + 1006; }
__attribute__((noinline)) int leaf_07(int x) { return x + 1007; }
__attribute__((noinline)) int leaf_08(int x) { return x + 1008; }
__attribute__((noinline)) int leaf_09(int x) { return x + 1009; }
__attribute__((noinline)) int leaf_10(int x) { return x + 1010; }
__attribute__((noinline)) int leaf_11(int x) { return x + 1011; }
__attribute__((noinline)) int leaf_12(int x) { return x + 1012; }
__attribute__((noinline)) int leaf_13(int x) { return x + 1013; }
__attribute__((noinline)) int leaf_14(int x) { return x + 1014; }
__attribute__((noinline)) int leaf_15(int x) { return x + 1015; }

int main() {
    uint8_t bits;
    uint8_t noise[2]; // tail noise
    klee_make_symbolic(&bits, sizeof(bits), "bits");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int r = 0;

    // 4-level balanced binary tree using top 4 bits
    uint8_t path = (bits >> 4) & 0x0F; // 16 possible leaves
    switch (path) {
        case 0:  r = leaf_00(bits); break;
        case 1:  r = leaf_01(bits); break;
        case 2:  r = leaf_02(bits); break;
        case 3:  r = leaf_03(bits); break;
        case 4:  r = leaf_04(bits); break;
        case 5:  r = leaf_05(bits); break;
        case 6:  r = leaf_06(bits); break;
        case 7:  r = leaf_07(bits); break;
        case 8:  r = leaf_08(bits); break;
        case 9:  r = leaf_09(bits); break;
        case 10: r = leaf_10(bits); break;
        case 11: r = leaf_11(bits); break;
        case 12: r = leaf_12(bits); break;
        case 13: r = leaf_13(bits); break;
        case 14: r = leaf_14(bits); break;
        case 15: r = leaf_15(bits); break;
    }

    // Tail noise: DFS gets stuck here after first leaf
    for (int i = 0; i < 2; i++) {
        if (noise[i] & 0x01) r++;
        if (noise[i] & 0x02) r++;
        if (noise[i] & 0x04) r++;
        if (noise[i] & 0x08) r++;
        if (noise[i] & 0x10) r++;
        if (noise[i] & 0x20) r++;
        if (noise[i] & 0x40) r++;
        if (noise[i] & 0x80) r++;
    }

    return r;
}
